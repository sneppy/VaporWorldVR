#include "runnable_thread.h"

#include <unistd.h>
#include <pthread.h>

#include <unordered_map>


namespace VaporWorldVR
{
	/* Map used to globally register threads. */
	using ThreadsMap = ::std::unordered_map<int, class RunnableThreadImpl*>;


	/**
	 * @brief Implementation of the RunnableThread interface that uses pthreads.
	 *
	 * @see RunnableThread
	 */
	class RunnableThreadImpl final : public RunnableThread
	{
	public:
		/**
		 * @brief Construct a new RunnableThreadImpl.
		 *
		 * @param inRunnable The runnable to bind to this thread
		 */
		RunnableThreadImpl(Runnable* inRunnable);

		// ---------------------------
		virtual void start() override;
		virtual void join() override;
		// --------------------------

		/**
		 * @brief Returns the RunnableThreadImpl instance with the given thread
		 * Id.
		 *
		 * @param tid The thread id to find
		 * @return Ptr to RunnableThreadImpl or null
		 */
		static FORCE_INLINE RunnableThreadImpl* findThreadById(int tid)
		{
			auto it = threads.find(tid);
			return it != threads.end() ? it->second : nullptr;
		}

	protected:
		/* Map of threads, indexed by tid. */
		static ThreadsMap threads;

		/* The pthread thread. */
		pthread_t thread;

		/**
		 * @brief The pthread entry function.
		 *
		 * @param payload The RunnableThreadImpl instance
		 * @return nullptr
		 */
		static void* pthreadStart(void* payload);
	};


	// ================================
	// RunnableThreadImpl static values
	// ================================
	ThreadsMap RunnableThreadImpl::threads;


	// =============================
	// RunnableThread implementation
	// =============================
	RunnableThread::RunnableThread(Runnable* inRunnable)
		: runnable{inRunnable}
		, tid{-1}
		, state{State_Created}
		, name{"UnnamedThread"}
	{
		// Set runnable thread
		VW_ASSERT(runnable != nullptr);
		runnable->thread = this;
	}

	RunnableThread::~RunnableThread()
	{
		VW_ASSERT(runnable != nullptr);
		VW_CHECKF(state == State_Terminated, "Destroying live thread @ %p", this);
		runnable->thread = this;
	}


	// =================================
	// RunnableThreadImpl implementation
	// =================================
	RunnableThreadImpl::RunnableThreadImpl(Runnable* inRunnable)
		: RunnableThread{inRunnable}
		, thread{}
	{}

	void RunnableThreadImpl::start()
	{
		// Create and start the pthread
		int err = pthread_create(&thread, NULL, pthreadStart, this);
		if (err != 0)
		{
			VW_LOG_ERROR("Failed to create thread '%s' (%d)", name.c_str(), err);
			return;
		}

		// Set thread name
		err = pthread_setname_np(thread, name.c_str());
		VW_CHECKF(err == 0, "Failed to set thread name (%d)", err);
	}

	void RunnableThreadImpl::join()
	{
		// Wait for runnable to finish
		int result = pthread_join(thread, NULL);
		VW_ASSERTF(result == 0, "Error occured while joining thread '%s'", name.c_str());
		state = State_Terminated;
	}

	void* RunnableThreadImpl::pthreadStart(void* payload)
	{
		RunnableThreadImpl* self = reinterpret_cast<decltype(self)>(payload);
		if (!self->runnable)
		{
			VW_LOG_WARN("Invalid task for thread '%s'", self->getName().c_str());
			return nullptr;
		}
		self->state = State_Started;

		// Read the thread id and register runnable thread
		self->tid = gettid();
		RunnableThreadImpl::threads.insert({self->tid, self});

		// Run the runnable task
		self->state = State_Resumed;
		self->runnable->run();
		self->state = State_Paused;

		return nullptr;
	}


	RunnableThread* createRunnableThread(Runnable* runnable)
	{
		return new RunnableThreadImpl(runnable);
	}

	void destroyRunnableThread(RunnableThread* thread, bool forceQuit)
	{
		if (thread->getState() != RunnableThread::State_Terminated)
		{
			if (!forceQuit)
			{
				// Wait for thread to finish
				thread->join();
			}
		}

		delete thread;
	}

	RunnableThread* getCurrentRunnableThread()
	{
		// Find the RunnableThread with the current thread id
		return RunnableThreadImpl::findThreadById(gettid());
	}
} // namespace VaporWorldVR
