/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef PENDING_ACKNOWLEDGE_QUEUE_HPP
#define PENDING_ACKNOWLEDGE_QUEUE_HPP

#include "General.h"
#include "Mutex.hpp"
#include "Ip.h"
#include <queue>
#include <string>

class PendingAcknowledgeQueue
{
private:
	static const uint32 notTransmitted = ~uint32(0);

	class Element
	{
		// a single command
	private:
		uint16				id;
		sockaddr_in	address;
		std::string const	text;

	public:
		Element(uint16 myId, sockaddr_in const& myServerAddress, std::string const& myText)
			: id(myId), address(myServerAddress), text(myText)
		{}

		uint16 Id() const {return id;}
		const sockaddr_in& Address() const {return address;}
		std::string const& Text() const {return text;}
	};

	std::queue<Element>		queue;

	//configuration
	void (*sendFunction) (sockaddr_in const& /*destination*/, std::string const& /*command*/);
	const uint16			repeatPeriod_ticks;
	const uint16			maxLength; //zero is no limit
	const uint16			maxTransmissions;

	//operation
	mutable Mutex			mutex;
	uint16					ticksSinceSend;
	uint16					transmissions;

public:
	//myActionFunction must point to a genuine function
	// myMaxOutstanding must equal 1 if order is to be maintained
	PendingAcknowledgeQueue(
		void (*mySendFunction) (sockaddr_in const& /*destination*/, std::string const& /*text*/),
		uint16 myRepeatPeriod_ticks, 
		uint16 myMaxLength = 0, uint16 myMaxRepeats = 4)
		: sendFunction(mySendFunction),
		repeatPeriod_ticks(myRepeatPeriod_ticks), maxLength(myMaxLength), maxTransmissions(myMaxRepeats+1), 
		ticksSinceSend(0), transmissions(0)
	{}

	~PendingAcknowledgeQueue(void) {}

	void Add(uint16 id, sockaddr_in const& serverAddress, std::string const& command)
	{
		MutexHolder holder(mutex);

		if ((maxLength != 0) && (queue.size() >= maxLength))
		{
			//queue will be too long. Remove oldest element
			queue.pop();
		}

		Element element(id, serverAddress, command);

		queue.push(element);

		if (queue.size() == 1)	// send immediately
			Send();
	}

	void Acknowledged(uint16 id)
	{
		MutexHolder holder(mutex);

		if (queue.empty() || queue.front().Id() != id)
			return;

		queue.pop();

		if (!queue.empty())
			Send();
	}

	void Tick()
	{
		MutexHolder holder(mutex);
		if (queue.empty())
			return;

		if (ticksSinceSend++ < repeatPeriod_ticks)
			return;	//still waiting

		if (transmissions >= maxTransmissions)
		{
			//timed out - delete it
			queue.pop();
			ticksSinceSend = 0;
			transmissions = 0;

			if (queue.empty())
				return;
		}

		Send();	//repeat
	}

	bool IsEmpty() const	//must not be called when mutex is held
	{
		MutexHolder holder(mutex);
		return queue.empty();
	}

	void WaitUntilAllMessagesAcknowledged();

private:
	void Send() 
	{
		ticksSinceSend = 0;
		transmissions++;
		sendFunction(queue.front().Address(), queue.front().Text());
	}

};

#endif

