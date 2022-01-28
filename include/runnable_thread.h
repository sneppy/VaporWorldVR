#pragma once

#include <string>

#include "core_types.h"
#include "logging.h"


namespace VaporWorldVR
{
	class Runnable;
	class RunnableThread;


	/**
	 * @brief Interface to implement runnable tasks.
	 *
	 * @see RunnableThread
	 */
	class Runnable
	{
		friend RunnableThread;

	public:
		virtual ~Runnable() {}

		/**
		 * @brief Returns the thread this task is running on.
		 */
		FORCE_INLINE RunnableThread* getThread() const
		{
			return thread;
		}

		/**
		 * @brief The method called when the thread starts.
		 *
		 * Derived classes must provide an implementation for this method.
		 */
		virtual void run() = 0;

		/**
		 * @brief Called before run() to setup the runnable.
		 *
		 * Called from the containing thread.
		 */
		virtual void setup() {};

		/**
		 * @brief Called after run() to perform teardown.
		 *
		 * Called from the containing thread.
		 */
		virtual void teardown() {};

	protected:
		/* Pointer that points to the thread this task is running on. */
		RunnableThread* thread;
	};


	/**
	 * @brief Implements an interface to an underlying thread that can execute
	 * a @c Runnable task.
	 *
	 * @see Runnable
	 */
	class RunnableThread
	{
	public:
		/**
		 * @brief Describe the current state of the thread:
		 *
		 * - State_Created: the runnable thread has not started yet;
		 * - State_Started: the runnable thread has started, but the runnable
		 *                  method has not been eun yet;
		 * - State_Resumed: the runnable run method is executing;
		 * - State_Paused: execution has been paused;
		 * - State_Terminated: the thread has been terminated.
		 */
		enum State : uint8_t
		{
			State_Created,
			State_Started,
			State_Resumed,
			State_Paused,
			State_Terminated
		};

		/**
		 * @brief Construct a new RunnableThread and bound the given Runnable.
		 *
		 * @param inRunnable The Runnable to bind to this thread
		 */
		RunnableThread(Runnable* inRunnable);

		/**
		 * @brief Destroy the RunnableThread object, and unbind the Runnable
		 * object.
		 */
		virtual ~RunnableThread();

		/**
		 * @brief Called to start a thread. After starting the thread the
		 * Runnable's run method is executed.
		 *
		 * This method implementation is platform-dependant.
		 */
		virtual void start() = 0;

		/**
		 * @brief Called to wait for this thread to finish. The current thread
		 * will be blocked until the Runnable's run method returns.
		 *
		 * This method implementation is platform-dependent.
		 */
		virtual void join() = 0;

		/**
		 * @brief Returns the thread id.
		 *
		 * The value of the thread id is platform-dependent.
		 */
		FORCE_INLINE int getId() const
		{
			return tid;
		}

		/**
		 * @brief Returns the state of the thread.
		 */
		FORCE_INLINE int getState() const
		{
			return state;
		}

		/**
		 * @brief Returns the name of the thread.
		 */
		FORCE_INLINE std::string const& getName() const
		{
			return name;
		}

		/**
		 * @brief Sets the name of the thread.
		 *
		 * Must be called before starting the thread.
		 */
		FORCE_INLINE void setName(std::string const& newName)
		{
			VW_CHECKF(state == State_Created, "%s called after thread has already started", __func__);
			if (state == State_Created)
			{
				name = newName;
			}
		}

		/**
		 * @brief Return the runnable bound to this thread.
		 *
		 * @tparam RunnableT The runtime type of the Runnable.
		 */
		template<typename RunnableT>
		FORCE_INLINE RunnableT* getRunnable() const
		{
			static_assert(::std::is_base_of<Runnable, RunnableT>::value, "RunnableT must be a Runnable type");
			return dynamic_cast<RunnableT*>(runnable);
		}

	protected:
		/* The runnable bound to the thread. */
		Runnable* runnable;

		/* The thread id. */
		int tid;

		/* The current state of the thread. */
		State state;

		/* The name of the thread. */
		::std::string name;

		RunnableThread() = delete;
	};


	/**
	 * @brief Creates a RunnableThread and binds the given task with it.
	 *
	 * @param runnable The Runnable to run
	 * @return Ptr to the created thread
	 */
	RunnableThread* createRunnableThread(Runnable* runnable);

	/**
	 * @brief Destroy a RunnableThread created with createRunnableThread().
	 *
	 * @param runnable The RunnableThread to destroy
	 */
	void destroyRunnableThread(RunnableThread* thread, bool forceQuit = false);
} // namespace VaporWorldVR
