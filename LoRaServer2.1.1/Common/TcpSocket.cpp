/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "TcpSocket.hpp"
#include "DebugMonitor.hpp"
#include "Utilities.hpp"


bool TCP::Socket::IsNowConnected(bool myConnected, IP::SocketSet& socketSet) 	//called when the socket has been connected outside the class
{
	connected = myConnected;

	bool result = true;
	if (myConnected)
		result = AddToSocketSet(socketSet);
	else
		RemoveFromSocketSet();

	return result;
}

int TCP::Socket::Receive(uint8 receivedData[], uint16 maxMessage) const
{
	//lint --e{740}  (Info -- Unusual pointer cast (incompatible indirect types))
	int receivedLength = recv(SocketDescriptor(), (char*) receivedData, maxMessage, 0);

	if (receivedLength < 0)
	{
		int errorNumber = IP::Error::ReadLastError();

		if ((errorNumber == IP::Error::connectionReset) || (errorNumber == IP::Error::connectionAborted))
			return IP::Socket::disconnected;

		IP::lastError.Set("Unable to receive datagram");
		return IP::Socket::error;
	}

	return receivedLength;
}


bool TCP::Socket::Send(uint8 const data[], uint16 length)
{
	//lint --e{740}  (Info -- Unusual pointer cast (incompatible indirect types))
	if (send(SocketDescriptor(),(char const*)data,length,0) != length)
	{
		connected = false;
		IP::lastError.Set("Unable to transmit data");
		Close();
		return false;
	}
	return true;
}

void TCP::Socket::Disconnect()
{
	if (Debug::Print(Debug::verbose))
	{
		std::stringstream text;

		text << "TCP socket disconnecting from host " << AddressText(partnerAddress);

		Debug::Write(text);
	}
	Close();
	connected = false;
}

