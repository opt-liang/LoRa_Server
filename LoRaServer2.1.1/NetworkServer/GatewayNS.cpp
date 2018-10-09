/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "GatewayNS.hpp"
#include "GatewayMessageProtocol.hpp"
#include "LoRaRegion.hpp"
#include "LoRaDatabaseNS.hpp"
#include "GlobalDataNS.hpp"

const uint16 Gateway::maxNumberOfGPSReadingsUsedToFindAverage = 20;
const uint16 Gateway::minNumberOfGPSReadingsUsedToFindAverage = 3;
const sint16 Gateway::defaultTransmitPower_dBm = 27;
const sint16 Gateway::maxTransmitPower_dBm = 30;
const uint16 Gateway::defaultDelayToNetworkServer_ms = 350;
const uint16 Gateway::defaultDelayFromNetworkServer_ms = 350;

GatewayList::GatewayList(bool initialise)
	: BinarySearchVectorNV::List<Gateway, EuiType>(initialise)
	//lint --e{1566}  Warning -- member 'GatewayList::nonVolatile' (line 122, file C:\Build\LoRa\Gateway-GPS\Server\Gateway.hpp) might have been initialized by a separate function 
{}


void GatewayList::Add(EuiType eui, LoRa::Region region)
{
	Position invalidPosition;
	Gateway* gateway = new Gateway(eui, region, invalidPosition, true);

	BinarySearchVectorNV::List<Gateway, EuiType>::Add(gateway);
}


void GatewayList::Add(EuiType eui, LoRa::Region myRegion, Position const& myPosition, bool myAllowGpsToSetPosition)
{
	Gateway* gateway = new Gateway(eui, myRegion, myPosition, myAllowGpsToSetPosition);

	BinarySearchVectorNV::List<Gateway, EuiType>::Add(gateway);
}


void GatewayList::GatewaySeen(EuiType eui, sockaddr_in const* myPullIpAddress)
{
	//lint --e{429} (Warning -- Custodial pointer 'gateway' has not been freed or returned)
	if ((eui == invalidEui) || (eui == nullEui))
		return;	//invalid (null) gateway

	if (Debug::Print(Debug::verbose))
	{
		std::stringstream text;

		text << "Gateway " << AddressText(eui) << " seen";

		if (myPullIpAddress)
			text << " IP address " << AddressText(*myPullIpAddress);

		Debug::Write(text);
	}

	Gateway* gateway = GetById(eui);	// gateway is locked

	if (!gateway)
	{
		//newly visible gateway
		Add(eui, Global::GetDefaultRegion());

		gateway = GetById(eui);
	}

	//Gateway is now locked and in list
	if (myPullIpAddress)
	{
		gateway->PullIpAddress(*myPullIpAddress);
		gateway->Seen();	//Seen() could be called every time a message is Rx from GW but there is no need.
	}

	gateway->Unlock();
}


LoRa::Region GatewayList::GetRegion(EuiType eui) const
{
	//lint --e{429} (Warning -- Custodial pointer 'gateway' has not been freed or returned)
	if ((eui == invalidEui) || (eui == nullEui))
		return Global::GetDefaultRegion();	//invalid (null) gateway

	Gateway* gateway = GetById(eui);	// gateway is locked

	if (!gateway)
		return Global::GetDefaultRegion();

	LoRa::Region result = gateway->Region();

	gateway->Unlock();

	return result;
}


void Gateway::SetRegion(LoRa::Region r)
{
	region = r;
	Global::networkDatabase.UpdateGateway(Id(), r);
}


void Gateway::AllowGpsToSetPosition(bool allow)
{
	if (allowGpsToSetPosition == allow)
		return;

	allowGpsToSetPosition = allow;
	Global::networkDatabase.UpdateGateway(Id(), GetPosition());
	SendStatusToClients();

	Global::networkDatabase.UpdateGatewayAllowGpsToSetPosition(Id(), allowGpsToSetPosition);
}


void Gateway::SetConfiguredPosition(Position const& p)
{
	configuredPosition = p; 
	Global::networkDatabase.UpdateGateway(Id(), GetPosition());
	SendStatusToClients();
}



Position const& Gateway::ConfiguredPosition() const
{
	if (allowGpsToSetPosition && gpsPosition.Valid())
		return gpsPosition;
	else
		return configuredPosition;
}


void Gateway::Seen()
{
	bool timeToSendStatusToClients = statusSendTimer.Tick();

	if (timeToSendStatusToClients)
		SendStatusToClients();
}


void Gateway::StatusReceived(uint64 receiveTime, GatewayStatusType const& newStatus)
{
	frameCount += newStatus.count;

	if (nvStoreTimer.Tick())
	{
		Global::networkDatabase.UpdateGateway(Id(), frameCount);
		Global::networkDatabase.UpdateGateway(Id(), newStatus.time);
	}

	bool timeToSendStatusToClients = statusSendTimer.Tick();
	bool gpsPositionChanged = gpsPosition.Input(newStatus.position) && allowGpsToSetPosition;
	if (timeToSendStatusToClients || gpsPositionChanged)
	{
		Global::networkDatabase.UpdateGateway(Id(), GetPosition());
		SendStatusToClients();
	}
}


void Gateway::SendStatusToClients()
{
	statusSendTimer.Reset();

	JSON::String jsonString;

	jsonString.Open();
	jsonString.OpenStructuredObject("gw", false);

	jsonString.AddEui("eui",Id(), false);

	Position const& position = GetPosition();
	if (position.Valid())
		jsonString.AddPositionObject(gpsPosition.Valid() && allowGpsToSetPosition, position);

	jsonString.AddTextValue("loraregion", LoRa::RegionText(region));

	jsonString.Close();	//gw
	jsonString.Close();

	Global::applicationList.Send(Application::nullAppEui, Service::gatewayStatusServer, jsonString);
}


bool Gateway::TransmitFrame(WaitingFrame const& frame, uint32 gatewayReceiveTimestamp_us, MoteTransmitRecord const& upstreamFrameTransmit, uint8 transmissionWindow, bool isARepeatTransmission)
{
	JSON::String json;
	FormJsonFrameTransmitString(json, frame, gatewayReceiveTimestamp_us, upstreamFrameTransmit, transmissionWindow);

	if (!isARepeatTransmission && Debug::Print(Debug::verbose))
	{
		std::stringstream logText;

		logText << "TX JSON text = " << json << std::endl;

		Debug::Write(logText);
	}

	//pullResponse token should always be zero
	GatewayMessageProtocol::TransmitMessage(Global::messageProtocolSocket, GatewayMessageProtocol::pullResponse, this->PullIpAddress(), Id(), 0,
		(uint8 const*) json.c_str(), uint16(json.length()));

	return true;
}

void Gateway::FormJsonFrameTransmitString(JSON::String& output, WaitingFrame const& frame, uint32 gatewayReceiveTimestamp_us, MoteTransmitRecord const& upstreamFrameTransmit, uint8 transmissionWindow) const
{
	LoRa::DataRate const& transmitDataRate = LoRa::GetDownlinkDataRate(Region(), upstreamFrameTransmit.dataRate, transmissionWindow);
	uint32 frequency_Hz = LoRa::GetDownLinkFrequency_Hz(region, upstreamFrameTransmit.frequency_Hz.Value(), transmissionWindow);;

	output.Open();	//top

	output.OpenStructuredObject("txpk", false);

	uint32 transmitDelay_us = frame.IsJoinAcceptFrame() ? LoRa::moteJoinRequestWindowTime_us : LoRa::moteWindowTime_us[transmissionWindow];
	uint32 transmitTimestamp_us = gatewayReceiveTimestamp_us + transmitDelay_us;	//rollover may occur
	output.AddUnsignedValue("tmst", transmitTimestamp_us, false);

	output.AddFixedPointNumber("freq", true, frequency_Hz, 1000000UL);

	output.AddUnsignedValue("rfch", LoRa::defaultGatewayTransmitAntenna);
	output.AddSignedValue("powe", Global::gatewayTxPower_dBm);
	output.AddTextValue("modu", (transmitDataRate.modulation == LoRa::loRaMod) ? "LORA" : "FSK");
	output.AddDataRate(transmitDataRate);

	if (transmitDataRate.modulation == LoRa::loRaMod)
	{
		LoRa::CodeRate const& transmitCodeRate = LoRa::GetDownlinkCodeRate(upstreamFrameTransmit.codeRate, transmissionWindow);

		output.AddTextValue("codr", transmitCodeRate);
		output.AddBooleanValue("ipol", true);
	}
	else
		output.AddUnsignedValue("fdev", LoRa::defaultFskFrequencyDeviation);

	output.AddUnsignedValue("size", frame.Length());
	output.AddDataAsBase64("data", frame.Data(), frame.Length());

	output.Close();	//txpk
	output.Close();	//top
}

