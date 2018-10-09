/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "General.h"
#include "GlobalDataCS.hpp"
#include "JsonGenerate.hpp"
#include "TcpConnectionManager.hpp"

void Global::DeleteAllDataServerSpecific()
{
	moteList.DeleteAllElements();
	customerDatabase.EmptyTable("gateways");
	customerDatabase.EmptyTable("appdata");
	customerDatabase.EmptyTable("gatewayframerx");
	customerDatabase.EmptyTable("moteframetx");
}


void Global::InitialiseDataServerSpecific(void)
{
	moteList.Initialise();
}


void Global::ServerSpecificTick()
{
}


bool JSON::SendPrivate(MessageAddress const& destination, char const data[], uint16 length)
{
	bool result = false;
	const uint8* const unsignedData = reinterpret_cast<uint8 const*>(data);

	if (destination.ConnectionValid())
	{
		::TCP::Connection* connection = Global::tcpConnectionManager.GetById(destination.TcpConnectionId());	//connection is locked

		if (connection)
		{
			result = connection->Socket().Send(unsignedData, length);
			connection->Unlock();
		}
	}
	else
		result = Global::udpSocket.Send(destination.Address(), unsignedData, length+1);

	return result;
}

bool CommandLineInterface::Send(MessageAddress const& destination, char const data[], uint16 length)
{
	return JSON::Send(destination, data, length);
}


bool Global::OpenUdpSockets()
{
	if (!Global::udpSocket.Open(localIpReceivePort))
		return false;

	return true;
}

void Global::CloseUdpSockets()
{
	Global::udpSocket.Close();
}

