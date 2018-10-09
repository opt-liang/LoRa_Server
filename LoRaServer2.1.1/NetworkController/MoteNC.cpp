/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "MoteNC.hpp"
#include "Maths.hpp"
#include "JsonGenerate.hpp"
#include "GlobalDataNC.hpp"
#include "ExceptionClass.hpp"

//Used by join request
Mote::Mote(EuiType moteEui, EuiType appEui)
	: BinarySearchVector::ElementTemplate<Mote, EuiType>(moteEui), appEui(appEui),
	adrEnabled(false), algorithmContainer(*this), bestGatewayEui(invalidEui)
{
}


void MoteList::CreateMote(EuiType moteEui, EuiType appEui)
{
	Mote* mote = new Mote(moteEui, appEui);

	Add(mote);
}


void MoteList::ApplicationDataReceived(bool up, EuiType moteEui, ValidValueUint32 const& sequenceNumber, ValidValueUint16 const& token,
		LoRa::FrameApplicationData const& payload, MoteTransmitRecord const& transmitRecord, GatewayReceiveList const& gatewayReceiveList)
{
	if (!up)
		throw Exception("MoteList::ApplicationDataReceived called with incorrect direction value");

	Mote* mote = GetById(moteEui);

	if (!mote && Global::autoCreateMotes)
	{
		CreateMote(moteEui, Mote::defaultApplicationEui);

		mote = GetById(moteEui);
	}

	if (!mote)
	{
		if (Debug::Print(Debug::verbose))
		{
			std::stringstream text;

			text << "Application data received for unknown mote " << AddressText(moteEui);
			Debug::Write(text);
		}

		return;
	}

	mote->ApplicationDataReceived(up, sequenceNumber, token, payload, transmitRecord, gatewayReceiveList);
	mote->Unlock();
}


void Mote::ApplicationDataReceived(bool up, ValidValueUint32 const& sequenceNumber, ValidValueUint16 token,
	LoRa::FrameApplicationData const& payload, MoteTransmitRecord const& transmitRecord, GatewayReceiveList const& gatewayReceiveList)
{
	if (!up)
		return;

	adrEnabled = transmitRecord.adrEnabled;

	if (!Global::applicationList.Exists(ApplicationEui()))
	{
		if (Debug::Print(Debug::minor))
		{
			std::stringstream text;

			text << "Upstream metadata received for unknown application: Mote :" << AddressText(Id()) << " App :" << AddressText(ApplicationEui());
			Debug::Write(text);
		}
		return;
	}

	mostRecentTransmission = transmitRecord;
	EuiType newGatewayEui = gatewayReceiveList.BestGatewayEui();

	if (newGatewayEui != invalidEui)
		bestGatewayEui = newGatewayEui;

	algorithmContainer.NewData(sequenceNumber, transmitRecord, gatewayReceiveList);
}


bool MoteList::ReceiveMoteServiceCommand(EuiType eui, WordStore& commandWordStore)
{
	Mote* mote = GetById(eui);

	if (!mote)
		return false;

	bool result;
	try
	{
		result = mote->ReceiveServiceCommand(commandWordStore);
	}
	catch (...)
	{
		mote->Unlock();
		throw;
	}

	mote->Unlock();
	return result;
}


void Mote::AcknowledgeReceived(bool app)
{
	JSON::String jsonString;
	
	jsonString.Open();
	jsonString.AddAcknowledgementReceived(Id(), app, false);
	jsonString.Close();

	Global::applicationList.Send(appEui, Service::userDataServer, jsonString);
}


void Mote::QueueLengthQueryReceived(bool app)
{
	JSON::String jsonString;
	
	jsonString.Open();
	jsonString.AddQueueLengthQuery(Id(), app, false);
	jsonString.Close();

	Global::applicationList.Send(appEui, Service::downstreamServer, jsonString);
}


void Mote::QueueLengthReceived(bool app, uint16 length)
{
	JSON::String jsonString;
	
	jsonString.Open();
	jsonString.AddQueueLength(Id(), app, length, false);
	jsonString.Close();

	Global::applicationList.Send(appEui, app ? Service::userDataServer : Service::macCommandServer, jsonString);
}


void MoteList::ReminderReceived(EuiType eui, uint16 algorithmId)
{
	Mote* mote = GetById(eui);

	if (!mote)
		return;

	mote->ReminderReceived(algorithmId);

	mote->Unlock();
}


LoRa::ValidRegion Mote::GetRegion() const
{
	return Global::gatewayList.GetRegion(bestGatewayEui);
}

