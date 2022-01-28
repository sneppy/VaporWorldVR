#pragma once


namespace VaporWorldVR
{
	/**
	 * @brief Implements a mutex, used to protect critical sections of a
	 * parallel program.
	 *
	 * Call createMutex() to create a new Mutex.
	 */
	class Mutex
	{
	public:
		virtual ~Mutex() {};

		/**
		 * @brief Blocks the thread execution until the lock can be acquired.
		 */
		virtual void lock() = 0;

		/**
		 * @brief Releases a lock acquired with lock().
		 */
		virtual void unlock() = 0;
	};


	/**
	 * @brief Creates a new Mutex object.
	 */
	Mutex* createMutex();

	/**
	 * @brief Destroys a Mutex created with createMutex().
	 *
	 * @param mutex The Mutex to destroy
	 */
	void destroyMutex(Mutex* mutex);
} // namespace VaporWorldVR
