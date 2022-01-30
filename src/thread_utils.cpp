#include "mutex.h"
#include "event.h"

#include <errno.h>
#include <pthread.h>

#include "logging.h"


namespace VaporWorldVR
{
	class MutexImpl;
	class EventImpl;


	/**
	 * @brief Platform-dependent implementation of Mutex.
	 *
	 * Uses pthread.
	 */
	class MutexImpl final : public Mutex
	{
		friend EventImpl;

	public:
		MutexImpl();
		~MutexImpl() override;
		virtual void lock() override;
		virtual void unlock() override;

	protected:
		/* The pthread mutex resource. */
		pthread_mutex_t mutex;

		/* Initialized flag, false if errors occured during initialization. */
		bool initd;
	};


	/**
	 * @brief Platform-dependent implement of Event.
	 *
	 * Uses pthread.
	 */
	class EventImpl final : public Event
	{
	public:
		EventImpl();
		~EventImpl() override;
		virtual void wait(Mutex* mutex) override;
		virtual void notifyAll() override;
		virtual void notifyOne() override;

	protected:
		/* The pthread condition. */
		pthread_cond_t cond;

		/* Initialized flag, false if errors occured during initialization. */
		bool initd;
	};


	// ================================
	// MutexImpl implementation
	// ================================
	MutexImpl::MutexImpl()
		: mutex{}
		, initd{false}
	{
		// Create pthread mutex
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
		int err = pthread_mutex_init(&mutex, &attr);
		pthread_mutexattr_destroy(&attr);
		if (err != 0)
		{
			VW_LOG_ERROR("Failed to initialize pthread mutex @ %p with error (%d)", &mutex, err);
			return;
		}

		initd = true;
	}

	MutexImpl::~MutexImpl()
	{
		if (initd)
		{
			// Destroy pthread resource
			pthread_mutex_destroy(&mutex);
			initd = false;
		}
	}

	void MutexImpl::lock()
	{
		VW_CHECK(initd);
		[[maybe_unused]] int err = pthread_mutex_lock(&mutex);
		VW_CHECKF(err == 0, "Failed to lock pthread mutex @ %p with error (%d)", &mutex, err);
	}

	void MutexImpl::unlock()
	{
		VW_CHECK(initd);
		[[maybe_unused]] int err = pthread_mutex_unlock(&mutex);
		VW_CHECKF(err == 0, "Failed to unlock pthread mutex @ %p with error (%d)", &mutex, err);
	}


	Mutex* createMutex()
	{
		return new MutexImpl();
	}

	void destroyMutex(Mutex* mutex)
	{
		delete mutex;
	}


	// ====================
	// Event implementation
	// ====================
	Event::Event()
		: state{State_NotifyNone}
		, numWaiting{0}
	{}


	// =========================
	// EventImple implementation
	// =========================
	EventImpl::EventImpl()
		: Event{}
		, cond{}
		, initd{false}
	{
		// Init the condition resource
		int err = pthread_cond_init(&cond, NULL);
		if (err != 0)
		{
			VW_LOG_ERROR("Failed to create pthread cond @ %p with error (%d)", &cond, err);
			return;
		}

		// Set initialized flag
		initd = true;
	}

	EventImpl::~EventImpl()
	{
		VW_CHECKF(numWaiting == 0, "Event destroyed, but %u clients were still waiting", numWaiting);

		if (initd)
		{
			// Release pthread resource
			pthread_cond_destroy(&cond);
			initd = false;
		}
	}

	void EventImpl::wait(Mutex* mutex)
	{
		VW_ASSERT(mutex != nullptr);
		VW_ASSERT(dynamic_cast<MutexImpl*>(mutex) != nullptr);
		VW_CHECKF(pthread_mutex_trylock(&(dynamic_cast<MutexImpl*>(mutex)->mutex)) == EBUSY,
		          "Event condition not protected, make sure to acquire the mutex lock before waiting");

		bool status = false;
		do
		{
			// Add to waiting queue
			numWaiting++;

			switch (state)
			{
			case State_NotifyOne:
				// Even if there are other threads waiting, we notify only one
				state = State_NotifyNone;
			case State_NotifyAll:
				status = true;
				break;

			case State_NotifyNone:
			{
				// Reinterpret cast is safe, in debug we assert if the dynamic cast fails anyway
				auto* mutexImpl = reinterpret_cast<MutexImpl*>(mutex);

				// Wait for signal
				pthread_cond_wait(&cond, &(mutexImpl->mutex));
			}
			break;

			default:
			{
				VW_LOG_ERROR("Invalid event state '%d'", state);
			}
			break;
			}

			// Remove from waiting queue
			numWaiting--;
		} while (!status);

		if (numWaiting == 0)
		{
			// Reset event state for future use
			state = State_NotifyNone;
		}
	}

	void EventImpl::notifyAll()
	{
		// Set state to trigger all and broadcast signal
		state = State_NotifyAll;
		[[maybe_unused]] int err = pthread_cond_broadcast(&cond);
		VW_CHECKF(err == 0, "Failed to broadcast pthread cond @ %p with error (%d)", &cond, err);
	}

	void EventImpl::notifyOne()
	{
		// Set state to trigger one and send signal
		state = State_NotifyOne;
		[[maybe_unused]] int err = pthread_cond_signal(&cond);
		VW_CHECKF(err == 0, "Failed to signal pthread cond @ %p with error (%d)", &cond, err);
	}


	Event* createEvent()
	{
		return new EventImpl{};
	}

	void destroyEvent(Event* event)
	{
		delete event;
	}
} // namespace VaporWorldVR
