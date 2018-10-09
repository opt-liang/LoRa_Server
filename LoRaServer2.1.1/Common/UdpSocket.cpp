/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "UdpSocket.hpp"

int UDP::Socket::Receive(sockaddr_in& source, uint8 receivedData[], uint16 maxMessage) const
{
	SocketAddressLengthType sourceSize = sizeof(source);

	int receivedLength;
	int errorNumber = 0;

	do
	{
		//lint --e{740}  (Info -- Unusual pointer cast (incompatible indirect types))
		receivedLength = recvfrom(SocketDescriptor(), (char*) receivedData, maxMessage, 0, (sockaddr*) &source, &sourceSize);

		if (receivedLength < 0)
			errorNumber = IP::Error::ReadLastError();

		if (errorNumber == IP::Error::wouldBlock)
			receivedLength = 0;
	}
	while (receivedLength < 0 && errorNumber == IP::Error::connectionReset);

	if (receivedLength < 0)
		IP::lastError.Set("Unable to receive datagram");

	return receivedLength;
}

bool UDP::Socket::Send(sockaddr_in const& destination, uint8 const data[], uint16 length) const
{
	//lint --e{740}  (Info -- Unusual pointer cast (incompatible indirect types))
	if (sendto(SocketDescriptor(),(char const*)data,length,0,(sockaddr const*)&destination, sizeof(destination)) != length)
	{
		IP::lastError.Set("Unable to transmit datagram");
		return false;
	}
	return true;
}

