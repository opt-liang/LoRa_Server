/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "JoinControllerNS.hpp"
#include "GlobalDataNS.hpp"
#include "Utilities.hpp"


/*
Request (as opposed to Request controller) functions 
*/
JoinController::Request::Request(JoinController& myController, LoRa::ReceivedFrame const& frame, uint64 myServerReplyTime_ms)
	: BinarySearchVector::ElementTemplate<Request, EuiType>(LoRa::Read8ByteValue(frame.Data() + LoRa::macHeaderLength + euiBytes)),
		controller(myController), 
		appEui(LoRa::Read8ByteValue(frame.Data() + LoRa::macHeaderLength)),
		serverReplyTime_ms(myServerReplyTime_ms),
		deviceNonce(LoRa::Read2ByteValue(frame.Data() + LoRa::macHeaderLength + 2 * euiBytes))
{
	ReceivedRequest(frame, true);
}

void JoinController::DeleteAllElements()
{
	list.DeleteAllElements();
	addressAllocator.Reset();
}


void JoinController::Request::ReceivedRequest(LoRa::ReceivedFrame const& frame, bool first)
{
	bestGateway.ReceivedFrame(first ? LoRa::first : LoRa::duplicate,
		frame.gatewayReceiveTime, frame.gatewayReceiveTimestamp_us, frame.signalToNoiseRatio_cB, frame.signalStrength_cBm,
		frame.GatewayEui(), frame.channel, frame.rfChain, frame.frequency_Hz, frame.dataRate,
		frame.codeRate, frame.AdrEnabled());
}

void JoinController::Request::ReceivedAccept(bool accept)
{
	int finish_accept_ignored;
	if (Debug::Print(Debug::verbose))
	{
		std::stringstream text;
		text << "JoinController received accept " << (accept ? "" : "(reject) ") << "for Mote " << AddressText(MoteEui());
		Debug::Write(text);
	}

	Application::Element* application = Global::applicationList.GetById(appEui);

	if (!application)
	{
		if (Debug::Print(Debug::monitor))
		{
			std::stringstream text;
			text << "JoinController accept unable to find application " << AddressText(appEui);
			Debug::Write(text);
			return;
		}
	}

	//test to find whether the mote already exists in moteList.  If it does, delete it
	Mote* mote = Global::moteList.GetById(MoteEui());	//mote is returned locked
	if (mote)
	{
		mote->Unlock();
		Global::moteList.DeleteById(MoteEui());
	}

	uint32 testMoteNetworkAddress = controller.addressAllocator.Request(MoteEui());

	if (testMoteNetworkAddress != LoRa::invalidNetworkAddress)
	{
		moteNetworkAddress = testMoteNetworkAddress;

		JSON::String jsonString;

		jsonString.Open();
		jsonString.AddJoinDetailsNtoA(MoteEui(), AppEui(), moteNetworkAddress, deviceNonce, false);
		jsonString.Close();

		application->Send(Service::joinServer, jsonString);
	}
	application->Unlock();
}

void JoinController::Request::ReceivedComplete(LoRa::Frame const& frame, LoRa::CypherKey const& networkKey)
{
	if (Debug::Print(Debug::verbose))
	{
		std::stringstream text;
		text << "JoinController received complete for Mote " << AddressText(MoteEui());
		Debug::Write(text);
	}

	if (!moteNetworkAddress.Valid())
		return; //cannot operate when network address is unknown

	if (Global::moteList.Exists(MoteEui()))
		Global::moteList.DeleteById(MoteEui());	//delete old mote with this address

	Global::moteList.CreateMote(MoteEui(), appEui, moteNetworkAddress, networkKey);
	moteNetworkAddress.SetInvalid();	//ownership has passed from the request to the mote

	//lint -e{838}  (Info -- Previously assigned value to variable 'mote' has not been used)
	Mote* mote = Global::moteList.GetById(MoteEui());
	if (!mote)
		return;

	mote->SetBestGateway(bestGateway);
	mote->SendFrameToMote(frame, true, 1);	//Join accept message is not repeated
	mote->Unlock();

	Global::transmitQueue.Add(MoteEui(), serverReplyTime_ms, 0);
}


/*
Controller functions 
*/
void JoinController::ReceivedRequest(LoRa::ReceivedFrame const& frame)
{
	//lint --e{429} (Warning -- Custodial pointer 'request' has not been freed or returned)
	if (frame.Length() != LoRa::Frame::joinRequestBytes)
		return;

	EuiType appEui  = LoRa::Read8ByteValue(frame.Data() + LoRa::macHeaderLength);
	EuiType moteEui = LoRa::Read8ByteValue(frame.Data() + LoRa::macHeaderLength + euiBytes);
	uint16 deviceNonce = LoRa::Read2ByteValue(frame.Data() + LoRa::macHeaderLength + 2 * euiBytes);

	MutexHolder holder(mutex);
	Request* request = static_cast<Request*>(list.GetById(moteEui));

	if (request)
	{
		//Request already exists - either the frame is a 2nd copy or mote is out of sequence
		bool is2ndCopy = request->DeviceNonce() == deviceNonce && request->AppEui() == appEui;

		if (Debug::Print(Debug::monitor))
		{
			std::stringstream text;

			text << "Received " << (is2ndCopy ? "duplicate" : "out of sequence") << " join request from Mote " << AddressText(moteEui);
			Debug::Write(text);
		}

		if (is2ndCopy)
			request->ReceivedDuplicateRequest(frame);

		request->Unlock();

		if (!is2ndCopy)
			list.DeleteById(moteEui);	// not a repeat so  mote is out of sequence
	}
	else
	{
		//NO existing request

		if (!Global::applicationList.Exists(appEui))
		{
			if (Debug::Print(Debug::monitor))
			{
				std::stringstream text;

				text << "Received join request from Mote " << AddressText(moteEui) << " for non-existent Application " << AddressText(appEui);
				Debug::Write(text);
			}
			return;
		}

		uint32 replyDelay_ms =  (LoRa::moteJoinRequestWindowTime_us / 1000) - (Global::gatewayToNetworkServerMaxDelay_ms + Global::networkServerToGatewayMaxDelay_ms) - Global::timeThreadTickPeriod_ms;
		request = new Request(*this, frame, frame.ThisServerReceiveTime_ms() + replyDelay_ms);

		list.Add(request);
		request = 0;

		Application::Element* application = Global::applicationList.GetById(appEui);
		if (!application)
			return;	// this will only happen if the application has been deleted since this function was called

		JSON::String jsonString;
		jsonString.Open();
		jsonString.AddJoinRequestNtoA(frame, false);
		jsonString.Close();

		application->Send(Service::joinServer, jsonString);
		application->Unlock();
	}
}

void JoinController::ReceivedAccept(EuiType moteEui, bool accept)
{
	MutexHolder holder(mutex);
	Request* request = static_cast<Request*>(list.GetById(moteEui));

	if (!request)
		return;

	request->ReceivedAccept(accept);
	request->Unlock();
}


void JoinController::ReceivedComplete(EuiType moteEui, LoRa::Frame const& frame, LoRa::CypherKey const& networkKey)
{
	MutexHolder holder(mutex);
	Request* request = static_cast<Request*>(list.GetById(moteEui));

	if (!request)
		return;

	request->ReceivedComplete(frame, networkKey);
	request->Unlock();

	list.DeleteById(moteEui);
}

