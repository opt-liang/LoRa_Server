/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef JOIN_CONTROLLER_HPP
#define JOIN_CONTROLLER_HPP

#include "General.h"
#include "LoRa.hpp"
#include "Mutex.hpp"
#include "ValueWithValidity.hpp"
#include "MessageAddress.hpp"
#include "Utilities.hpp"
#include <list>

class JoinController
{
	//lint --e{1502} (Warning -- defined object 'Global::joinController' of type 'JoinController' has no nonstatic data members)
	class QueuedRequest
	{
	private:
		EuiType const		eui;
		uint64 const		creationTime_ms;

	public:
		QueuedRequest(EuiType myEui)
			:eui(myEui), creationTime_ms(GetMsSinceStart())
		{}

		EuiType Eui() const {return eui;}
		uint64 CreationTime_ms() const {return creationTime_ms;}
	};

	class RequestStore	//holds requests for a short time
	{
	private:
		uint64 const				holdTime_ms;
		std::list<QueuedRequest>	list;

		uint64 FindEraseTime_ms() const {return GetMsSinceStart() - holdTime_ms;}	//requests with CreationTime() older than this are deleted

	public:
		RequestStore(uint64 myHoldtime_ms = LoRa::moteJoinRequestWindowTime_us / 1000)
			: holdTime_ms(myHoldtime_ms)
		{}

		~RequestStore()
		{list.clear();}

		void Add(EuiType myEui)
		{
			QueuedRequest dummy(myEui);
			list.push_back(dummy);	//object copy
		}

		void Cleanup()
		{
			uint64 eraseTime_ms =  FindEraseTime_ms();

			while (!list.empty() && list.front().CreationTime_ms() < eraseTime_ms)
				list.pop_front();
		}


		bool InList(EuiType eui) const
		{
			std::list<QueuedRequest>::const_iterator it = list.begin();

			for (;it != list.end(); it++)
				if (it->Eui() == eui)
					return true;

			return false;
		}
	};


private:
	RequestStore		requestStore;

public:
	JoinController()
	{}

	void ReceivedRequest(uint8 const data[]);	//length of data must already have been checked
	void ReceivedDetails(EuiType moteEui, EuiType appEui, uint32 moteNetworkAddress, uint16 deviceNonce) const;

private:
};


#endif

