/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "General.h"
#include "GlobalDataNS.hpp"
#include "JsonGenerate.hpp"
#include "TcpConnectionManager.hpp"
#include "LoRa.hpp"
#include "LoRaRegion.hpp"
#include "LoRaIpPorts.hpp"


namespace Global
{
	bool SendPrivate(MessageAddress const& destination, uint8 const data[], uint16 length, bool useMessageProcotolSocket);
}

void Global::DeleteAllDataServerSpecific()
{
	moteList.DeleteAllElements();
	joinController.DeleteAllElements();
	gatewayList.DeleteAllElements();
}


void Global::InitialiseDataServerSpecific(void)
{
	moteList.Initialise();
	gatewayList.Initialise();
}


void Global::ServerSpecificTick()
{
}


bool JSON::SendPrivate(MessageAddress const& destination, char const data[], uint16 length)
{
	return SendPrivate(destination, reinterpret_cast<uint8 const*>(data), length, false);
}


bool JSON::SendPrivate(MessageAddress const& destination, uint8 const data[], uint16 length, bool useMessageProcotolSocket)
{
	bool result = false;

	if (destination.ConnectionValid())
	{
		::TCP::Connection* connection = Global::tcpConnectionManager.GetById(destination.TcpConnectionId());	//connection is locked

		if (connection)
		{
			result = connection->Socket().Send(data, length);
			connection->Unlock();
		}
	}
	else
	{
		UDP::Socket& selectedSocket = useMessageProcotolSocket ? Global::messageProtocolSocket : Global::jsonSocket;

		result = selectedSocket.Send(destination.Address(), data, length+1);
	}
	return result;
}


bool CommandLineInterface::Send(MessageAddress const& destination, char const data[], uint16 length)
{
	return JSON::Send(destination, data, length);
}


LoRa::Region Global::GetDefaultRegion()
{
	return Global::defaultGatewayRegion.Value();	//region name is assumed to be correct format (checked on write)
}


LoRa::DataRate const& Global::Get2ndWindowDataRate(LoRa::Region region)
{
	switch (region)
	{
	case LoRa::americas902:		return Global::dataRateWindow1Americas902;
	case LoRa::china779:		return Global::dataRateWindow1China779;
	case LoRa::europe433:		return Global::dataRateWindow1Eu433;
	case LoRa::europe863:		return Global::dataRateWindow1Eu863;
	default:					return Global::dataRateWindow1Eu863;
	}
}


bool Global::CreateProvisionedMote(EuiType moteEui, EuiType const& appEui, uint32 networkAddress, LoRa::CypherKey const& authenticationKey)
{
	if (!joinController.ReserveNetworkAddress(moteEui, networkAddress))
		return false;

	Global::moteList.CreateMote(moteEui, appEui, networkAddress, authenticationKey);
	return true;
}


bool Global::DeleteProvisionedMote(EuiType moteEui)
{
	Mote* mote = Global::moteList.GetById(moteEui);
	if (!mote)
		return false;

	uint32 networkAddress = mote->NetworkAddress();
	mote->Unlock();

	joinController.ReleaseNetworkAddress(networkAddress);

	Global::moteList.DeleteById(moteEui);
	return true;
}


template<> std::string ConfiguredValueNonIntegerTemplate<LoRa::DataRate>::ValueText() const
{
	return value;
}


template<> bool ConfiguredValueNonIntegerTemplate<LoRa::DataRate>::IsValid(std::string const& text) const
{
	LoRa::DataRate test = text;
	return test.Valid();
}


template<> void ConfiguredValueNonIntegerTemplate<LoRa::DataRate>::ReadValue(std::string const& text)
{
	value = text;
}


template<> std::string ConfiguredValueNonIntegerTemplate<LoRa::Region>::ValueText() const
{
	return LoRa::RegionText(value);
}


template<> bool ConfiguredValueNonIntegerTemplate<LoRa::Region>::IsValid(std::string const& text) const
{
	LoRa::Region region = LoRa::ReadRegion(text);

	return region < LoRa::numberOfRegions ? true :false;
}


template<> void ConfiguredValueNonIntegerTemplate<LoRa::Region>::ReadValue(std::string const& text)
{
	value = LoRa::ReadRegion(text);
}


bool Global::OpenUdpSockets()
{
	if (!Global::messageProtocolSocket.Open(LoRa::UDP::gatewayMessageProtocolPort))
		return false;

	if (!Global::jsonSocket.Open(LoRa::UDP::jsonPort))
		return false;

	if (!Global::messageProtocolSocket.AddToSocketSet(Global::udpSocketSet))
		return false;

	if (!Global::jsonSocket.AddToSocketSet(Global::udpSocketSet))
		return false;

	return true;
}


void Global::CloseUdpSockets()
{
	Global::messageProtocolSocket.Close();
	Global::jsonSocket.Close();
}

