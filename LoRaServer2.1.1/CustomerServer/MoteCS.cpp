/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "MoteCS.hpp"
#include "JsonGenerate.hpp"
#include "Application.hpp"
#include "GlobalDataCS.hpp"
#include "ValueWithValidity.hpp"


void MoteList::CreateMote(EuiType myEui, EuiType myApp)
{
	DeleteById(myEui);

	if (Debug::Print(Debug::monitor))
	{
		std::stringstream text;

		text << "Creating Mote " << AddressText(myEui) << 
			" for app " << AddressText(myApp);

		Debug::Write(text);
	}

	Mote* mote = new Mote(myEui, myApp);

	Add(mote);
}


void MoteList::ApplicationDataReceived(bool up, EuiType eui, ValidValueUint32 const& sequenceNumber, ValidValueUint16 const& token,
		LoRa::FrameApplicationData const& payload, MoteTransmitRecord const& transmitRecord, GatewayReceiveList const& gatewayReceiveList)
{
	Mote* mote = GetById(eui);

	if (!mote)
	{
		CreateMote(eui, Mote::defaultApplicationEui);

		mote = GetById(eui);
	}

	if (!mote)
	{
		if (Debug::log.Print(Debug::verbose))
		{
			std::stringstream text;

			text << "Application data received from unknown mote " << AddressText(eui);
			Debug::Write(text);
		}
		return;
	}

	mote->ApplicationDataReceived(up, sequenceNumber, token, payload, transmitRecord, gatewayReceiveList);
	mote->Unlock();
}


void MoteList::AppDataTransmittedToGateway(EuiType eui, uint16 token)
{
	Mote* mote = GetById(eui);

	if (!mote)
		return;

	mote->AppDataTransmittedToGateway(token);
	mote->Unlock();
}


void Mote::ApplicationDataReceived(bool up, ValidValueUint32 const& sequenceNumber, ValidValueUint16 token,
	LoRa::FrameApplicationData const& payload, MoteTransmitRecord const& transmitRecord, GatewayReceiveList const& gatewayReceiveList)
{
	if (up)
		Global::applicationDataOutput.UpstreamApplicationDataReceived(Id(), sequenceNumber, payload, transmitRecord, gatewayReceiveList);
	else
		SendAppData(payload);
}


bool MoteList::SendAppData(EuiType moteEui, LoRa::FrameApplicationData const& data)
{
	Mote* mote = GetById(moteEui);

	if (!mote)
	{
		if (Debug::log.Print(Debug::verbose))
		{
			std::stringstream text;

			text << "Application data received from unknown mote " << AddressText(moteEui);
			Debug::Write(text);
		}
		return false;
	}

	mote->SendAppData(data);
	mote->Unlock();
	return true;
}


void MoteList::QueueLengthReceived(EuiType eui, uint16 length)
{
	Mote* mote = GetById(eui);

	if (!mote)
		return;

	mote->QueueLengthReceived(length);
	mote->Unlock();
}


void Mote::AcknowledgeReceived()
{
}


void Mote::QueryQueueLength()
{
	JSON::String jsonString;
	
	jsonString.Open();
	jsonString.AddQueueLengthQuery(Id(), true, false);
	jsonString.Close();

	Global::applicationList.Send(appEui, Service::downstreamServer, jsonString);
}


void Mote::ResetDetected()
{
}


void Mote::DownlinkController::SendToMote(LoRa::FrameApplicationData const& data)
{
	static const ValidValueUint32 invalidSequenceNumber;
	MoteTransmitRecord invalidTransmitRecord;
	GatewayReceiveList invalidGatewayList;

	JSON::String jsonString;
	
	jsonString.Open();
	jsonString.AddAppObject(false, mote.Id(), invalidValueUint32, ++txToken, data, invalidTransmitRecord, invalidGatewayList, false);
	jsonString.Close();

	Global::applicationList.Send(mote.ApplicationEui(), Service::downstreamServerMask, jsonString);
}


void Mote::DownlinkController::RequestQueueLength()
{
	JSON::String jsonString;
	
	jsonString.Open();
	jsonString.AddQueueLengthQuery(mote.Id(), true, false);
	jsonString.Close();

	Global::applicationList.Send(mote.ApplicationEui(), Service::downstreamServer, jsonString);
}

