/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "TransmitQueue.hpp"
#include "MoteNS.hpp"


void Transmit::Queue::Tick(uint64 now_ms)
{
	Opportunity transmitOpportunity = static_cast<Opportunity>(list.Tick(now_ms));

	if (transmitOpportunity.Valid())
	{
		Mote* mote = moteList.GetById(transmitOpportunity.MoteEui());

		//***** if mote is returned it is LOCKED
		if (mote)
		{
			mote->FrameSendOpportunity(transmitOpportunity.MoteTransmissionWindow());
			mote->Unlock();		//UNLOCK mote *********************
		}
	}
}

