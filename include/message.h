#pragma once

#include <variant>

#include "utility.h"
#include "mutex.h"
#include "event.h"


namespace VaporWorldVR
{
	/* Base class for messages exchanged between application modules. */
	struct Message {};


	/**
	 * @brief This class provides an API to exchange messages between separate
	 * modules (e.g. application -> render thread).
	 *
	 * Messages sent by the same recipient are guaranteed to be delivered and
	 * processed in the same order they were sent. There's no such guarantee
	 * between messages sent by different recipients.
	 *
	 * @tparam TargetT The target class (should be the class that implements
	 *                 the API)
	 * @tparam MessagesT The list of Messages this target accepts
	 */
	template<typename TargetT, typename ...MessagesT>
	class MessageTarget
	{
		using MessageVarT = ::std::variant<MessagesT...>;

	public:
		/**
		 * @brief Construct a new MessageTarget object.
		 */
		MessageTarget()
			: beforeFirst_NoInit{}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wuninitialized"
			, tail{&beforeFirst}
#pragma clang diagnostic pop
			, mutex{createMutex()}
			, event{createEvent()}
		{
			beforeFirst.next = nullptr;
		}

		/**
		 * @brief Destroy the MessageTarget object.
		 */
		~MessageTarget()
		{
			// TODO: Empty queue

			destroyEvent(event);
			destroyMutex(mutex);
		}

		/**
		 * @brief Returns true if the message queue is empty.
		 */
		FORCE_INLINE bool isEmpty() const
		{
			return tail == &beforeFirst;
		}

		/**
		 * @brief Posts a message to the target object.
		 *
		 * The target object will process the message the next time it calls
		 * flushMessages().
		 *
		 * @param msg The message to post
		 */
		void postMessage(auto&& msg)
		{
			mutex->lock();
			{
				// Push to queue
				tail = tail->next = new MessageWrapper{FORWARD(msg)};

				// Notify target
				event->notifyOne();
			}
			mutex->unlock();

			// TODO: Wait ack?
		}

		/**
		 * @brief Process the message queue.
		 *
		 * @param blocking If true and the queue empty, it will block execution
		 *                 and wait for new messages
		 */
		void flushMessages(bool blocking = false)
		{
			mutex->lock();
			{
				while (blocking && isEmpty())
				{
					// Block on waiting for new messages
					event->wait(mutex);
				}

				// Process queue
				auto* it = &beforeFirst;
				while (it->next)
				{
					// Pop message
					auto* wrapper = it->next;
					it->next = wrapper->next;

					// Process message
					::std::visit([this](auto const& msg) -> void {

						static_cast<TargetT*>(this)->processMessage(msg);
					}, wrapper->msg);

					// Destroy the wrapper and the message
					delete wrapper;
				}
				tail = it;
			}
			mutex->unlock();
		}

	protected:
		/* Wraps a message with a pointer to the next message in the queue. */
		struct MessageWrapper
		{
			/* This message. */
			MessageVarT msg;

			/* Pointer to next message in queue. */
			MessageWrapper* next;
		};

		union
		{
			/* Used to prevent initialization of member. */
			int beforeFirst_NoInit;

			/* Head of the queue, msg should never be accessed. */
			MessageWrapper beforeFirst;
		};

		/* Tail of the queue. */
		MessageWrapper* tail;

		/* Mutex that provides protected access to the queue. */
		Mutex* mutex;

		/* Event used to signal incoming messages. */
		Event* event;
	};
} // namespace VaporWorldVR
