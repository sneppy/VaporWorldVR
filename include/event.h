#pragma once

#include "core_types.h"


namespace VaporWorldVR
{
	class Mutex;


	/**
	 * @brief This class implements an interface to wait for specific events to
	 * occur.
	 *
	 * A thread may call wait() to wait for a specific event to be triggered by
	 * another thread. Multiple threads can wait for the same event and the
	 * master thread can choose to notify only one or all waiting events.
	 *
	 * This class should not be instantiated directly. Call createEvent() to
	 * create a new event.
	 */
	class Event
	{
	public:
		virtual ~Event() {};

		/**
		 * @brief Called to wait for an event.
		 *
		 * @param mutex The mutex that protects the event condition.
		 */
		virtual void wait(Mutex* mutex) = 0;

		/**
		 * @brief Broadcast a signal to wake up all waiting threads.
		 */
		virtual void notifyAll() = 0;

		/**
		 * @brief Wake up a single thread that is waiting on this event.
		 *
		 */
		virtual void notifyOne() = 0;

	protected:
		/* Describe the state of the event. An event that has not triggered yet
		   will be in the state State_NotifyNone. An event that has triggered
		   for a single client will be in the state State_NotifyOne, until one
		   client consumes it. An event that has triggered for all clients will
		   be in the state State_NotifyAll until the number of clients waiting
		   drops to zero. */
		enum State
		{
			State_NotifyNone,
			State_NotifyAll,
			State_NotifyOne
		};

		/* The current state of the event. */
		State state;

		/* The number of clients waiting for this event. */
		uint32_t numWaiting;

		Event();
	};


	/**
	 * @brief Returns a new Event object.
	 */
	Event* createEvent();

	/**
	 * @brief Destroys an Event created with createEvent().
	 *
	 * @param event The Event to destroy
	 */
	void destroyEvent(Event* event);
} // namespace VaporWorldVR
