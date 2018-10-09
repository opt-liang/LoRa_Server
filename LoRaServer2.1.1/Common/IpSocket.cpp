/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "IpSocket.hpp"
#include "IpSocketSet.hpp"

#include <stdio.h>
#include <memory.h>

sockaddr_in GetNullIpAddress();

namespace
{
	bool initialised = IP::Initialise();
}

const sockaddr_in IP::nullAddress = GetNullIpAddress();

IP::Error IP::lastError;

bool IP::Initialise()
{
#ifdef _MSC_VER
	const uint16 maxWindowsSocketVersion = 0x0202;
	/* The WSAStartup function initiates use of the Winsock DLL by a process. */
	WSADATA wsaData;
	int error = WSAStartup(maxWindowsSocketVersion, &wsaData);

	if (error != 0)
	{
		lastError.Set("Could not initiate use of the Winsock DLL");
		return false;
	}

	/* Confirm that the WinSock DLL supports 2.2.*/
	/* Note that if the DLL supports versions greater    */
	/* than 2.2 in addition to 2.2, it will still return */
	/* 2.2 in wVersion since that is the version we      */
	/* requested.                                        */

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) 
	{
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		lastError.Set("Could not find a usable version of Winsock.dll");
		return false;
	}
#endif
	return true;
}


bool IP::IsOnThisMachine(sockaddr_in const& address)
{
	if (!IsValid(address))
		return false;

	char hostName[maxHostNameLength+1];
	int error = gethostname(hostName,maxHostNameLength);

	if (error != 0)
	{
		lastError.Set("Unable to read host name");
		return false;
	}

	if (address.sin_addr.s_addr == htonl(INADDR_LOOPBACK))
		return true;

	const int sockType = SOCK_STREAM | SOCK_DGRAM;
	struct addrinfo hints;
	memzero(&hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = sockType;
	hints.ai_protocol = int(IPPROTO_IP);

	struct addrinfo* interfaceList;
	error = getaddrinfo(hostName, 0, &hints, &interfaceList);	//if successful, must always be followed by a call to freeaddrinfo

	if (error != 0 && interfaceList == 0)
	{
		lastError.Set("Unable to read host address");
		return false;
	}
	
	bool result = false;
	for (struct addrinfo* ptr = interfaceList; ptr != NULL; ptr = ptr->ai_next) 
	{
		if (ptr->ai_family != AF_INET)
			continue;	//only want IPv4 as yet

		//lint --e{740}  (Info -- Unusual pointer cast (incompatible indirect types))
		sockaddr_in* addressPtr = ((sockaddr_in*) (ptr->ai_addr));

		if (addressPtr->sin_addr.s_addr == address.sin_addr.s_addr)
		{
			result= true;
			break;
		}
	}
	freeaddrinfo(interfaceList);	/* interfaceList is no longer needed */
	return result;
}

bool IP::Socket::Open(uint16 newPortNumber, sockaddr_in const& newInterfaceAddress)
{
	localPortNumber = newPortNumber;
	interfaceAddress = newInterfaceAddress;

	return Reopen();
}

bool IP::Socket::Open(uint16 newPortNumber)
{
	localPortNumber = newPortNumber;
	SetInvalid(interfaceAddress);
	return Reopen();
}

SOCKET IP::Socket::Reopen(IP::Socket::Type socketType, sockaddr_in const& localInterfaceAddress, uint16 localPortNumber)
{
	char hostName[IP::maxHostNameLength+1];
	int error = gethostname(hostName,IP::maxHostNameLength);

	if (error != 0)
	{
		IP::lastError.Set("Unable to read host name");
		return INVALID_SOCKET;
	}

	int sockType = (socketType == IP::Socket::tcp) ? SOCK_STREAM : SOCK_DGRAM;
	SOCKET socketDescriptor = INVALID_SOCKET;
	if (localInterfaceAddress.sin_addr.s_addr != htonl(INADDR_ANY))
	{
		struct addrinfo hints;
		memzero(&hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = sockType;
		hints.ai_protocol = int((socketType ==  IP::Socket::tcp) ? IPPROTO_TCP : IPPROTO_UDP);

		struct addrinfo* interfaceList;
		error = getaddrinfo(hostName, 0, &hints, &interfaceList);	//if successful, must always be followed by a call to freeaddrinfo

		if (error != 0 && interfaceList == 0)
		{
			IP::lastError.Set("Unable to read host address");
			return INVALID_SOCKET;
		}

		for (struct addrinfo* ptr = interfaceList; ptr != NULL; ptr = ptr->ai_next) 
		{
			if (ptr->ai_family != AF_INET)
				continue;	//only want IPv4 as yet

			if (ptr->ai_socktype != sockType)
				continue;

			//lint --e{740}  (Info -- Unusual pointer cast (incompatible indirect types))
			sockaddr_in* addressPtr = ((sockaddr_in*) (ptr->ai_addr));

			if ((localInterfaceAddress.sin_addr.s_addr != htonl(INADDR_ANY)) && (addressPtr->sin_addr.s_addr != localInterfaceAddress.sin_addr.s_addr))
				continue;	//local interface address is specified but does not match socket's local interface address

			socketDescriptor = socket(AF_INET, sockType, 0);
			if (socketDescriptor == INVALID_SOCKET)
				continue;

			addressPtr->sin_port = htons(localPortNumber);
			if (bind(socketDescriptor, ptr->ai_addr, int(ptr->ai_addrlen)) == 0)
				break;                  /* Success */

			closesocket(socketDescriptor);
			socketDescriptor = INVALID_SOCKET;
		}
		freeaddrinfo(interfaceList);		/* interfaceList is no longer needed */
	}
	else
	{
		socketDescriptor = socket(AF_INET, sockType, 0);
		if (socketDescriptor == INVALID_SOCKET)
		{
			IP::lastError.Set("Unable to set socket descriptor");
			return INVALID_SOCKET;
		}

		sockaddr_in myAddress;
		SetInvalid(myAddress);
		myAddress.sin_family = AF_INET;
		myAddress.sin_addr.s_addr = htonl(INADDR_ANY);
		myAddress.sin_port = htons(localPortNumber);

		//lint --e{740}  (Info -- Unusual pointer cast (incompatible indirect types))
		if (bind(socketDescriptor, (sockaddr *) &myAddress, sizeof(myAddress)) != 0)
			socketDescriptor = INVALID_SOCKET;
	}

	if (socketDescriptor == INVALID_SOCKET)
		IP::lastError.Set("Unable to bind socket");

	return socketDescriptor;
}

bool IP::Socket::Reopen()
{
	if (!initialised)	// in case Open is called before initialised is initialised
		initialised = Initialise();

	sd = IP::Socket::Reopen(type, interfaceAddress, localPortNumber);
	
	return sd != INVALID_SOCKET;
}

void IP::Socket::Close(SOCKET sd)
{
	if (sd != INVALID_SOCKET) 
		closesocket(sd);
}

void IP::Socket::Close()
{
	SetActiveInSocketSet(false);

	Close(sd);
	sd = INVALID_SOCKET;
}

sockaddr_in IP::Socket::GetLocalAddress() const
{
	sockaddr_in result;
	socklen_t length = sizeof(result);

	if (getsockname(sd, (sockaddr *)&result, &length) != 0)
		memzero(&result);
	
	return result;
}

uint16 IP::Socket::GetPortNumber() const
{
	sockaddr_in address = GetLocalAddress();

	return ntohs(address.sin_port);
}

bool IP::ReadAddressText(char const text[], uint32& output)
{
	const int elements = 4;
	unsigned read[elements];

	memset(read, 0, sizeof(unsigned) * elements);

	int readCount = sscanf(text,"%u.%u.%u.%u",&read[3],&read[2],&read[1],&read[0]);

	output = 0;
	if (readCount != elements)
		return false;

	for (int i = elements -1; i >= 0; i--)
	{
		output <<= 8;
		output |= uint8(read[i]);
	}

	return true;
}

bool IP::ReadAddressText(char const text[], sockaddr_in& output) //converts dotted decimal into structure
{
		uint32 readValue;
		if (!ReadAddressText(text, readValue))
			return false;

		SetInvalid(output);
		output.sin_family = AF_INET;
		output.sin_addr.s_addr = htonl(readValue);

		char const* cptr = text;

		for (; *cptr != '\0' && *cptr != ':'; cptr++)
			;	//empty loop

		if (*cptr == '\0')
			return true;	// no port

		cptr++; // beyond ':'
		while (isspace(*cptr))
			cptr++;

		readValue = 0;

		//lint --e{571}  (Warning -- Suspicious cast)
		for (;  isdigit(*cptr); cptr++)
			readValue = readValue * 10 + uint32(*cptr - '0');

		output.sin_port = htons(uint16(readValue));

		return true;
}

bool IP::Socket::SetActiveInSocketSet(bool active)
{
	bool result = true;

	if (!socketSet)	// no socket set configured
		return !active;

	if (active)
		result = socketSet->Add(*this);
	else
		socketSet->Remove(*this);

	return result;
}

bool IP::Socket::AddToSocketSet(IP::SocketSet& set)
{
	socketSet = &set;
	return SetActiveInSocketSet(true);
}

void IP::Socket::RemoveFromSocketSet()
{
	SetActiveInSocketSet(false);
	socketSet = 0;
}

bool IP::Socket::Validate() const
{
	bool good;

#ifdef _MSC_VER
	unsigned long store;
	good = ioctlsocket(sd, FIONREAD, &store) == 0;
#else
	good = fcntl(sd, F_GETFL) >= 0;
#endif

	return good;
}

int IP::Error::ReadLastError()
{
#ifdef _MSC_VER
	return WSAGetLastError();
#else
	return errno;
#endif
}


sockaddr_in GetNullIpAddress()	//only exists to keep lint and g++ happy
{
	sockaddr_in result;
	memzero(&result);
	return result;
}

