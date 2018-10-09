/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "MoteAS.hpp"
#include "Maths.hpp"
#include "JsonGenerate.hpp"
#include "Eui.hpp"
#include "LoRa.hpp"
#include "GlobalDataAS.hpp"
#include "ExceptionClass.hpp"

//Used by join request
Mote::Mote(EuiType moteEui, uint32 myNetworkAddress, EuiType appEui, LoRa::CypherKey const& myApplicationKey, uint16 myDeviceNonce)
	: BinarySearchVector::ElementTemplate<Mote, EuiType>(moteEui), networkAddress(myNetworkAddress), appEui(appEui), 
	applicationKey(myApplicationKey), applicationNonce(0), downstreamDataController(*this)
{
	UpdateApplicationNonce();
	uint32 networkId = LoRa::CalculateNetworkId(myNetworkAddress);
	sessionKey = LoRa::GenerateSessionKey(false, applicationKey, networkId, applicationNonce, myDeviceNonce);
}

//Read from activeMotes database table
Mote::Mote(EuiType moteEui, uint32 myNetworkAddress, EuiType myAppEui, LoRa::CypherKey const& myApplicationKey, LoRa::CypherKey const& myEncryptionSessionKey)
	: BinarySearchVector::ElementTemplate<Mote, EuiType>(moteEui), networkAddress(myNetworkAddress), appEui(myAppEui), 
	applicationKey(myApplicationKey), sessionKey(myEncryptionSessionKey), applicationNonce(0), downstreamDataController(*this)
{
	UpdateApplicationNonce();
}

//Used by auto join and provisioned
Mote::Mote(EuiType moteEui, uint32 myNetworkAddress, EuiType myAppEui, LoRa::CypherKey const& myEncryptionSessionKey)
	: BinarySearchVector::ElementTemplate<Mote, EuiType>(moteEui), networkAddress(myNetworkAddress), appEui(myAppEui), 
	sessionKey(myEncryptionSessionKey), applicationNonce(0), downstreamDataController(*this)
{
	//applicationKey is invalid
}


void MoteList::CreateMote(EuiType moteEui, uint32 myNetworkAddress, EuiType myAppEui, LoRa::CypherKey const& applicationKey, uint16 myDeviceNonce)
{
	DeleteById(moteEui);

	if (Debug::Print(Debug::verbose))
	{
		std::stringstream text;

		text << "Creating Mote " << AddressText(moteEui) << 
			" for app " << AddressText(myAppEui) << 
			" network address " << AddressText(myNetworkAddress);

		Debug::Write(text);
	}

	Mote* mote = new Mote(moteEui, myNetworkAddress, myAppEui, applicationKey, myDeviceNonce);

	Add(mote);
}

void MoteList::CreateMote(EuiType moteEui, uint32 myNetworkAddress, EuiType myAppEui, LoRa::CypherKey const& myEncryptionSessionKey)
{
	DeleteById(moteEui);

	Mote* mote = new Mote(moteEui, myNetworkAddress, myAppEui, myEncryptionSessionKey);

	Add(mote);

	if (Debug::Print(Debug::verbose))
	{
		std::stringstream text;

		text << "Mote " << AddressText(mote->Id()) << " created";
		Debug::Write(text);
	}
}


void MoteList::ApplicationDataReceived(bool up, EuiType moteEui, ValidValueUint32 const& sequenceNumber, ValidValueUint16 const& token,
		LoRa::FrameApplicationData const& payload, MoteTransmitRecord const& transmitRecord, GatewayReceiveList const& gatewayReceiveList)
{
	Mote* mote = GetById(moteEui);

	if (!mote && Global::autoCreateMotes)
	{
		CreateMote(moteEui, uint32(moteEui), nullEui, Global::encryptionKeyDefault);

		mote = GetById(moteEui);
	}

	if (!mote)
	{
		if (Debug::log.Print(Debug::verbose))
		{
			std::stringstream text;

			text << "Application data received from unknown mote " << AddressText(moteEui);
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
	if (up)
	{
		LoRa::FrameApplicationData transmittedData;
		transmittedData.SetLength(payload.Length());
		transmittedData.Port(payload.Port());

		bool decrypt = payload.Port() != LoRa::macCommandPort;

		if (decrypt)
			LoRa::DecryptPayload(sessionKey,payload.Data(),payload.Length(),NetworkAddress(), up, sequenceNumber, transmittedData.DataNonConst());

		ForwardApplicationData(up, sequenceNumber, token, decrypt ? transmittedData : payload, transmitRecord , gatewayReceiveList);
	}
	else
	{
		// Don't decrypt downlink data, sequence number is not yet known
		downstreamDataController.DownstreamApplicationDataReceived(payload, token);
	}
}


void Mote::ForwardApplicationData(bool up, ValidValueUint32 const& sequenceNumber, ValidValueUint16 const& token, LoRa::FrameApplicationData const& payload, MoteTransmitRecord const& transmitRecord, GatewayReceiveList const& gatewayList) const
{
	JSON::String jsonString;
	jsonString.Open();
	jsonString.AddAppObject(up, Id(), sequenceNumber, token, payload, transmitRecord, gatewayList, false);
	jsonString.Close();	//top

	Service::Mask mask = up ? Service::userDataServer : Service::downstreamServer;
	Global::applicationList.Send(appEui, mask, jsonString);
}


void Mote::JoinedNetwork(uint16 deviceNonce)
{
	SendJoinComplete(deviceNonce);

	JSON::String jsonString;

	jsonString.Open();
	jsonString.AddJoinNotificationAtoN(Id(), appEui, false);
	jsonString.Close();

	Global::applicationList.Send(appEui, Service::joinMonitor, jsonString);
}



void Mote::SendJoinComplete(uint16 deviceNonce)
{
	uint8 uncyphered[LoRa::maxDataBytes];
	uint8* current = uncyphered;

	uint32 networkId = LoRa::CalculateNetworkId(networkAddress);
	LoRa::CypherKey networkSessionKey = LoRa::GenerateSessionKey(true, applicationKey, networkId, applicationNonce, deviceNonce);

	*(current++) = uint8(LoRa::joinAcceptFrame) << LoRa::Frame::typeShift;
	current = LoRa::Write3ByteValue(current, applicationNonce);
	current = LoRa::Write3ByteValue(current, networkId);
	current = LoRa::Write4ByteValue(current, networkAddress);
	current = LoRa::Write1ByteValue(current, 3);	//This ununused octet, specified by the Semtech LoRa MAC spec as zero, is transmitted as 3 to give forward compatibility with the LoRaWAN protocol
	current = LoRa::Write1ByteValue(current, 0);

	uint16 authenticedBytes = current-uncyphered;
	LoRa::GenerateJoinFrameIntegrityCode(applicationKey, uncyphered, authenticedBytes, current);
	current += LoRa::micBytes;

	if (Debug::Print(Debug::verbose))
	{
		std::stringstream logText;

		logText << std::endl << "Mote " << AddressText(Id())  << " accepted" << std::endl <<
			"Network address:" << AddressText(networkAddress) << std::endl <<
			"Network session key: " << networkSessionKey.CastToString() << std::endl <<
			"App session key: " << sessionKey.CastToString() << std::endl <<
			std::hex << 
			"App nonce : " << applicationNonce << std::endl <<
			"Dev nonce : " << deviceNonce << std::endl;

		logText << ConvertBinaryArrayToHexTextBlock("Join complete uncyphered frame", uncyphered, current - uncyphered) << std::endl;
		Debug::Write(logText);
	}

	LoRa::Frame frame;
	frame.AppendByte(uint8(LoRa::joinAcceptFrame) << LoRa::Frame::typeShift);
	//encrypt
	uint16 cypherBytes = (current - uncyphered) - LoRa::macHeaderLength;
	LoRa::CryptJoinServer(applicationKey, &uncyphered[LoRa::macHeaderLength], cypherBytes, frame.FirstUnusedByte());
	frame.IncreaseLength(cypherBytes);

	if (Debug::Print(Debug::verbose))
	{
		std::stringstream logText;

		logText << ConvertBinaryArrayToHexTextBlock("Join complete frame", frame.Data(), frame.Length()) << std::endl;

		Debug::Write(logText);
	}

	JSON::String jsonString;

	jsonString.Open();
	jsonString.AddJoinComplete(Id(), networkSessionKey, frame);
	jsonString.Close();

	Global::applicationList.Send(appEui, Service::downstreamServer, jsonString);
}


void Mote::DataTransmittedToGateway(bool app, uint16 token)
{
	JSON::String jsonString;
	
	jsonString.Open();
	jsonString.AddMessageSent(Id(), app, token, false);
	jsonString.Close();

	Global::applicationList.Send(appEui, Service::userDataServer, jsonString);
}


void Mote::AcknowledgeReceived(bool app)
{
	JSON::String jsonString;
	
	jsonString.Open();
	jsonString.AddAcknowledgementReceived(Id(), false);
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


void Mote::ResetDetected()
{
	downstreamDataController.Clear();
	InformUpstreamServerOfMoteReset();
}


void Mote::UpdateApplicationNonce()
{
	applicationNonce = Maths::Random32() & 0x00FFFFFFUL; //mask to 24 bits
}


void Mote::InformUpstreamServerOfMoteReset()
{
	JSON::String jsonString;

	jsonString.Open();
	jsonString.AddResetDetected(Id(), false);
	jsonString.Close();

	Global::applicationList.Send(appEui, Service::fullMask & ~Service::downstreamServer, jsonString);	//send to everything except the downstream server
}


void Mote::RequestDownstreamFrameSequenceNumber()
{
	JSON::String jsonString;

	jsonString.Open();
	jsonString.AddSequenceNumberRequest(Id(), false);
	jsonString.Close();

	Global::applicationList.Send(appEui, Service::downstreamServer, jsonString);
}

