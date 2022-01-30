#pragma once

#include <variant>

#include "utility.h"
#include "mutex.h"
#include "event.h"


namespace VaporWorldVR
{
	enum MessageWait
	{
		MessageWait_None = 0,
		MessageWait_Received = 1 << 0,
		MessageWait_Processed = 1 << 1
	};


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
			, eventSent{createEvent()}
			, eventRcvd{createEvent()}
			, eventProc{createEvent()}
		{
			beforeFirst.next = nullptr;
		}

		/**
		 * @brief Destroy the MessageTarget object.
		 */
		~MessageTarget()
		{
			VW_CHECKF(isEmpty(), "Some messages still in queue, but target is being destroyed");

			for (auto* it = &beforeFirst; it->next;)
			{
				// Destroy all messages in queue
				auto* wrapper = it->next;
				VW_CHECKF(wrapper->refCount == 0, "Destroying message @ %p with %u live refs", wrapper,
				          wrapper->refCount);

				it->next = wrapper->next;
				delete wrapper;
			}

			// Destroy mutex and events
			destroyEvent(eventProc);
			destroyEvent(eventRcvd);
			destroyEvent(eventSent);
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
		 * @tparam MessageT The type of the message to send
		 * @param msg The message to post
		 * @param flags Used to request acks from the target
		 * @{
		 */
		template<typename MessageT>
		void postMessage(MessageT const& msg, int sendFlags = MessageWait_None)
		{
			postMessage_Impl(msg, sendFlags);
		}

		template<typename MessageT>
		void postMessage(MessageT&& msg, int sendFlags = MessageWait_None)
		{
			postMessage_Impl(::std::move(msg), sendFlags);
		}
		/// @}

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
					eventSent->wait(mutex);
				}

				// Process queue
				auto* it = &beforeFirst;
				while (it->next)
				{
					acquireRef(it->next);

					// Pop message
					auto* wrapper = it->next;
					it->next = wrapper->next;

					if (wrapper->reqFlags & MessageWait_Received)
					{
						// Signal received event
						wrapper->ackFlags |= AckFlag_Received;
						eventRcvd->notifyOne();
					}

					// Process message
					::std::visit([this](auto&& msg) -> void {

						static_cast<TargetT*>(this)->processMessage(FORWARD(msg));
					}, wrapper->msg);

					if (wrapper->reqFlags & MessageWait_Processed)
					{
						// Signal processed event
						wrapper->ackFlags |= AckFlag_Processed;
						eventProc->notifyOne();
					}

					releaseRef(wrapper);
				}
				tail = it;
			}
			mutex->unlock();
		}

	protected:
		enum AckFlag
		{
			AckFlag_Received = 1 << 0,
			AckFlag_Processed = 1 << 1
		};

		/* Wraps a message with a pointer to the next message in the queue. */
		struct MessageWrapper
		{
			/* This message. */
			MessageVarT msg;

			/* Wether should block until received/processed. */
			int reqFlags = 0;

			/* Ack flags (used for both received and processed). */
			int ackFlags = 0;

			// TODO: maybe use an atomic, though we only write this in critical sections
			/* Number of live references. */
			uint32_t refCount = 0;

			/* Pointer to next message in queue. */
			MessageWrapper* next = nullptr;
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

		/* Event fired whenever a new message is posted. */
		Event* eventSent;

		/* Event fired when a message that requires a ack is received. */
		Event* eventRcvd;

		/* Event fired after a message that requires a ack is processed. */
		Event* eventProc;

		FORCE_INLINE void postMessage_Impl(auto&& msg, int sendFlags)
		{
			mutex->lock();
			{
				// Push to queue
				auto* wrapper = new MessageWrapper{FORWARD(msg), sendFlags};
				tail = tail->next = wrapper;

				// Notify target
				eventSent->notifyOne();

				if ((sendFlags & MessageWait_Received) == MessageWait_Received)
				{
					acquireRef(wrapper);
					while ((wrapper->ackFlags & AckFlag_Received) != AckFlag_Received)
					{
						// Wait for the received event to trigger and check again
						eventRcvd->wait(mutex);
					}
					releaseRef(wrapper);
				}

				if ((sendFlags & MessageWait_Processed) == MessageWait_Processed)
				{
					acquireRef(wrapper);
					while ((wrapper->ackFlags & AckFlag_Processed) != AckFlag_Processed)
					{
						// Wait for the processed event to trigger and check again
						eventProc->wait(mutex);
					}
					releaseRef(wrapper);
				}
			}
			mutex->unlock();
		}

		FORCE_INLINE void acquireRef(MessageWrapper* wrapper)
		{
			wrapper->refCount++;
		}

		FORCE_INLINE void releaseRef(MessageWrapper* wrapper)
		{
			wrapper->refCount--;
			if (wrapper->refCount == 0)
			{
				// Delete message
				delete wrapper;
			}
		}
	};
} // namespace VaporWorldVR
