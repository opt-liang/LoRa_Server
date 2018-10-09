/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "FrameReception.hpp"

#include "Application.hpp"
#include "GlobalDataNS.hpp"

void FrameReceptionRecord::Transmit()
{
	Mote* mote = Global::moteList.GetById(moteEui);

	if (!mote)
		return;

	LoRa::FrameApplicationData decryptedPayload;

	bool decrypt = payload.Port() == LoRa::macCommandPort;
	if (decrypt)
	{
		decryptedPayload.SetLength(payload.Length());
		decryptedPayload.Port(payload.Port());

		LoRa::DecryptPayload(mote->NetworkSessionKey(), payload.Data(), payload.Length(), mote->NetworkAddress(), true, payload.SequenceNumber(), decryptedPayload.DataNonConst());
	}

	EuiType appEui = mote->ApplicationEui();
	mote->Unlock();
	mote = 0;

	Application::Element* application = Global::applicationList.GetById(appEui);
	if (!application)
		return;

	application->Send(moteEui, true, payload.SequenceNumber(), decrypt ? decryptedPayload : payload, transmit, gatewayReceiveList);
	application->Unlock();
}


