#include "runnable_thread.h"

#include <unistd.h>
#include <pthread.h>


namespace VaporWorldVR
{
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

	protected:
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
		int result = pthread_create(&thread, NULL, pthreadStart, this);
		VW_ASSERTF(result == 0, "Failed to create thread '%s'", name.c_str());

		// Set thread name
		result = pthread_setname_np(thread, name.c_str());
		VW_CHECKF(result == 0, "Failed to set thread name")
	}

	void RunnableThreadImpl::join()
	{
		// Wait for runnable to finish
		int result = pthread_join(thread, NULL);
		VW_ASSERTF(result == 0, "Error occured while joining thread '%s'", name.c_str());
	}

	void* RunnableThreadImpl::pthreadStart(void* payload)
	{
		RunnableThreadImpl* self = reinterpret_cast<decltype(self)>(payload);
		if (!self->runnable)
		{
			VW_LOG_WARN("Invalid task for thread '%s'", self->getName().c_str());
			return nullptr;
		}

		// Read the thread id
		self->tid = gettid();

		// Run the runnable task
		self->runnable->run();
		return nullptr;
	}


	RunnableThread* createRunnableThread(Runnable* runnable)
	{
		return new RunnableThreadImpl(runnable);
	}
} // namespace VaporWorldVR
