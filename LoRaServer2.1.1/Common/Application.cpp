/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "Application.hpp"
#include "JsonString.hpp"
#include "JsonGenerate.hpp"
#include "GlobalData.hpp"
#include "ExceptionClass.hpp"


void Application::List::Initialise()
{
	BinarySearchVectorNV::List<Element, EuiType>::Initialise();

	if (!Exists(nullAppEui))	// need to create null app
		Add(nullAppEui, nullAppName, "");	// this is the normal situation - null app shouldn't be in the DB
}


bool Application::List::Add(EuiType eui, std::string const& name, std::string const& owner)
{
	if (Exists(eui))
		return false;

	Application::Element* app = new Element(eui, name, owner);

	Application::List::Add(app);

	return true;
}


Application::Element* Application::List::GetByName(std::string const& name, std::string const& owner) const	//returned client is locked
{
	BinarySearchVector::IndexType index = 0;

	MutexHolder holder(mutex);	//Lock BinarySearchVectorNV::List<Element, EuiType> mutex

	for (; index < Size(); index++)
	{
		Element* application =  GetByIndex(index);
		/*GetByIndex does not lock BinarySearchVectorNV::List<Application::Server*, EuiType> mutex.
		It does however lock the BinarySearchVector::List<Application::Server*, EuiType> mutex*/

		if (!application)
		{
			if (Debug::Print(Debug::major))
				Debug::Write("Unexpected null pointer return in Application::List::GetByName");
			continue;
		}

		if (application->Name() == name && application->Owner() == owner)
			return application;

		application->Unlock();
	}
	return 0;
}


EuiType Application::List::GetEui(std::string const& name, std::string const& owner) const
{
	Element* application = GetByName(name, owner);

	if (!application)
		return invalidEui;

	EuiType result = application->Id();
	application->Unlock();
	return result;
}


bool Application::List::Exists(std::string const& name, std::string const& owner) const
{
	Element* application = GetByName(name, owner);

	if (!application)
		return false;

	application->Unlock();
	return true;
}

bool Application::List::AddServer(EuiType eui, sockaddr_in const& address, bool activeConnection, Service::Mask serviceMask)
{
	Element* application = GetById(eui);

	if (!application)
		return false;

	application->AddServer(address, activeConnection, serviceMask);

	application->Unlock();
	return true;
}


bool Application::List::DeleteServer(EuiType eui, sockaddr_in const& address)
{
	Element* application = GetById(eui);

	if (!application)
		return false;

	application->DeleteServer(address);

	application->Unlock();
	return true;
}


bool Application::List::Send(EuiType eui, Service::Mask serviceMask, std::string const& text)
{
	Element* application = GetById(eui);

	if (!application)
		return false;

	application->Send(serviceMask, text);

	application->Unlock();

	return true;
}


void Application::Element::Send(EuiType moteEui, bool up, ValidValueUint32 const& sequenceNumber, ValidValueUint16 const& token, 
			LoRa::FrameApplicationData const& payload, MoteTransmitRecord const& transmitRecord, GatewayReceiveList const& gatewayReceiveList) const
{
	JSON::String appDataString;
	JSON::String moteTxMetaDataString;
	JSON::String gatewayRxMetaDataString;

	appDataString.AddUserData(payload, false);
	moteTxMetaDataString.AddMoteTxMetaData(transmitRecord, false);
	gatewayRxMetaDataString.AddGatewayRxMetaData(gatewayReceiveList, false);

	Service::Mask dataDestinationMask = 0;

	if (payload.Length() > 0)
	{
		dataDestinationMask |= payload.IsUserData() ? Service::userDataServer : 0;
		dataDestinationMask |= payload.IsMacCommand() ? Service::macCommandServer : 0;
	}

	Service::Mask destinationMask = dataDestinationMask;
	
	if (transmitRecord.Valid())
		destinationMask |= Service::moteTxServer;
	
	if (gatewayReceiveList.Size() > 0)
		destinationMask |= Service::gatewayRxServer;

	JSON::String startText;
	startText.Open();	//top
	startText.OpenStructuredObject("app", false);
	startText.AddEui("moteeui", moteEui, false);
	startText.AddDirection(true);

	if (sequenceNumber.Valid())
		startText.AddUnsignedValue("seqno", sequenceNumber.Value());

	if (token.Valid())
		startText.AddUnsignedValue("token", token.Value());

	for (uint i = 0; i < servers.Size(); i++)
	{
		Service::Mask sendMask = servers.GetServiceMask(i) & destinationMask;

		if (sendMask == 0)
			continue;

		JSON::String sendText = startText;

		if (sendMask & dataDestinationMask)
		{
			sendText.Comma();
			sendText += appDataString;
		}

		if (sendMask & Service::moteTxServer)
		{
			sendText.Comma();
			sendText += moteTxMetaDataString;
		}

		if (sendMask & Service::gatewayRxServer)
		{
			sendText.Comma();
			sendText += gatewayRxMetaDataString;
		}

		sendText.Close();	//app
		sendText.Close();	//top
		servers.Send(i, sendText);
	}
}


void Application::Element::Send(Service::Mask serviceMask, std::string const& text)
{
	for (uint i = 0; i < servers.Size(); i++)
	{
		Server const* server = servers.GetByIndex(i);

		if (!server)
			throw Exception(__LINE__, __FILE__, "Unexpected null pointer");

		if (serviceMask & server->ServiceMask())
			server->Send(text);
	}
}

