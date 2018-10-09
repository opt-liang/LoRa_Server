/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef TRANSMIT_QUEUE_HPP
#define TRANSMIT_QUEUE_HPP

#include "General.h"
#include "LoRa.hpp"
#include "Mutex.hpp"
#include "Ip.h"
#include "TimedQueueTemplate.hpp"

#include <list>

class MoteList;

namespace Transmit
{
	class Opportunity : public TimedQueueTemplate::ElementParent
	{
	private:
		EuiType		moteEui;
		uint8		moteTransmissionWindow;		//this must be consistent with serverTransmitTime_ms;

	public:
		Opportunity(EuiType myMoteEui, uint64 serverTransmitTime_ms, uint8 myMoteTransmissionWindow)
			: TimedQueueTemplate::ElementParent(serverTransmitTime_ms), 
			moteEui(myMoteEui), moteTransmissionWindow(myMoteTransmissionWindow)
		{}

		Opportunity() 
			:moteEui(invalidEui), moteTransmissionWindow(0)
		{} // only suitable when element is invalid

		uint32 MoteTransmissionWindow() const {return moteTransmissionWindow;}
		EuiType MoteEui() const {return moteEui;}
		bool Valid() const {return moteEui != invalidEui;}
	};


	class Queue
	{
	private:
		::MoteList&										moteList;
		TimedQueueTemplate::List<Opportunity>			list;	//newest elements at the beginning

	public:
		Queue(::MoteList& myMoteList)
			: moteList(myMoteList)
		{}

		void Add(EuiType myEui, uint64 serverTransmitTime_ms, uint8 moteTransmitWindow)
		{
			Opportunity opportunity(myEui, serverTransmitTime_ms, moteTransmitWindow);

			list.Add(opportunity);
		}

		void Tick(uint64 now_ms);

		uint64 TimeOfNextTransmit_ms() const {return list.NextTargetTime_ms();}
	};
}

#endif

