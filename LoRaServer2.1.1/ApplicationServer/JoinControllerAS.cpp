/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "JoinControllerAS.hpp"
#include "GlobalDataAS.hpp"
#include "MessageAddress.hpp"

void JoinController::ReceivedRequest(uint8 const data[])
{
	if (Debug::Print(Debug::verbose))
	{
		std::stringstream debugText;
		debugText << "Received Join Request at " << GetMsSinceStart() <<"ms";
		Debug::Write(debugText);
	}

	EuiType moteEui = LoRa::Read8ByteValue(data + LoRa::macHeaderLength + euiBytes);

	//requestStore avoids a single join request, received via 2 gateways, being processed twice
	requestStore.Cleanup();	//remove old records
	if (requestStore.InList(moteEui))
		return;	//duplicate

	requestStore.Add(moteEui);

	EuiType appEui = LoRa::Read8ByteValue(data + LoRa::macHeaderLength);
	uint16 deviceNonce = LoRa::Read2ByteValue(data + LoRa::macHeaderLength + 2 * euiBytes);
	bool success = true;

	if (Debug::Print(Debug::verbose))
	{
		std::stringstream logText;

		logText << std::endl << std::endl << "JoinController::Request::ReceivedRequest " << AddressText(moteEui);
		
		if (Debug::Print(Debug::verbose))
			logText << " " << ConvertBinaryArrayToHexTextBlock("Mote join request frame", data, LoRa::Frame::joinRequestBytes);

		Debug::Write(logText);
	}


	LoRa::CypherKey applicationKey;
	if (!Global::applicationDatabase.FindJoinMoteApplicationKey(moteEui, appEui, applicationKey))
	{
		if (Debug::Print(Debug::monitor))
		{
			std::stringstream text;

			text << "Unable to join Mote " << AddressText(moteEui) << " to App " << AddressText(appEui) << ".  Mote is not registered with app.";
			Debug::Write(text);
		}
		success = false;
	}
	uint8 calculatedMic[LoRa::micBytes];
	LoRa::GenerateJoinFrameIntegrityCode(applicationKey, data, LoRa::Frame::joinRequestBytes - LoRa::micBytes, calculatedMic);

	if (success && (memcmp(data + LoRa::Frame::joinRequestBytes - LoRa::micBytes, calculatedMic, LoRa::micBytes) != 0))
	{
		if (Debug::Print(Debug::verbose))
		{
			std::stringstream text;

			text << "Join request from Mote " << AddressText(moteEui) << " to App " << AddressText(appEui) << " failed MIC";
			Debug::Write(text);
		}
		success = false;
	}

	if (success && (!Global::allowDuplicateMoteNonce && Global::applicationDatabase.MoteNonceKnown(moteEui, deviceNonce)))
	{
		if (Debug::Print(Debug::monitor))
		{
			std::stringstream text;

			text << "Rejecting mote " << AddressText(moteEui) << " because of duplicate nonce " << std::hex << deviceNonce;
			Debug::Write(text);
			}
		return;
	}

	JSON::String jsonString;

	jsonString.Open();
	jsonString.AddJoinAccept(moteEui, success);
	jsonString.Close();

	Global::applicationList.Send(appEui, Service::downstreamServer, jsonString);
}


void JoinController::ReceivedDetails(EuiType moteEui, EuiType appEui, uint32 moteNetworkAddress, uint16 deviceNonce) const
{
	if (Debug::Print(Debug::verbose))
	{
		std::stringstream text;
		text << "ReceivedDetails Mote " << AddressText(moteEui) << " at "<< GetMsSinceStart();
		Debug::Write(text);
	}
	if (Debug::Print(Debug::verbose))
	{
		std::stringstream logText;

		logText << std::endl << std::endl << "JoinController::Request::ReceivedDetails " << AddressText(moteEui);
		
		Debug::Write(logText);
	}

	LoRa::CypherKey applicationKey;
	if (!Global::applicationDatabase.FindJoinMoteApplicationKey(moteEui, appEui, applicationKey))
	{
		if (Debug::Print(Debug::monitor))
		{
			std::stringstream text;

			text << "Unable to find app key for Mote " << AddressText(moteEui);

			Debug::Write(text);
		}
		return;
	}

	Global::moteList.CreateMote(moteEui, moteNetworkAddress, appEui, applicationKey, deviceNonce);

	Mote* mote = Global::moteList.GetById(moteEui);

	if (!mote)
		return;
	
	mote->JoinedNetwork(deviceNonce);
	// send joint complete even if mote is already marked as joined - to avoid lock out

	mote->Unlock();
}


