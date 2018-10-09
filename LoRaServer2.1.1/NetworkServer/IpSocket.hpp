/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef IP_SOCKET_HPP
#define IP_SOCKET_HPP

#include "General.h"
#include "Ip.h"
#include <string>
#include <limits.h>

#ifdef _MSC_VER
	typedef int SocketAddressLengthType;
#else
	#define SOCKET int
	#define SOCKET_ERROR (-1)
	#define	INVALID_SOCKET (-1)
	#include <errno.h>
	#include <fcntl.h>
	typedef socklen_t SocketAddressLengthType;
	#define closesocket(sd) close(sd)
#endif


namespace IP
{
	const uint16 maxWindowsSocketVersion = 0x0202;
	const uint16 maxHostNameLength = 256;
	const uint16 maxReceivedMessage = 1024;
	const uint32 noErrorNumber = ~uint32(0);	//all ones

	bool ReadAddressText(char const text[], uint32& output); //converts dotted decimal into integer
	bool ReadAddressText(char const text[], struct sockaddr_in& output); //converts dotted decimal into structure

	bool Initialise();	//MUST be called at startup.  Automatically called by IP:::Socket::Open()

	class Error
	{
	public:
#ifdef _MSC_VER
		static const int wouldBlock = WSAEWOULDBLOCK;
		static const int connectionReset = WSAECONNRESET;
#else
		static const uint wouldBlock = EWOULDBLOCK;
		static const int connectionReset = ECONNRESET;
#endif

	private:
		std::string description;
		int errorNumber;
	public:
		Error(char const descriptionText[] = "")
			: description(descriptionText), errorNumber(ReadLastError())
		{}

		std::string const& Text() const {return description;}
		sint32 Number() const {return errorNumber;}

		void Set(char const descriptionText[]) {description = descriptionText; errorNumber = ReadLastError();}
		void Set(char const descriptionText[], sint32 errorNo) {description = descriptionText; errorNumber = errorNo;}

		static int ReadLastError();
	};

	extern Error	lastError;	//common to the namespace

	class SocketSet;

	class Socket
	{
		friend class SocketSet;
	public:
		const static int error = -1;
		const static int immediateReturn = -2;
		const static int disconnected = immediateReturn;	//only relevant to tcp sockets
		enum Type {tcp, udp};

	private:
		const Type			type;
		SOCKET				sd;
		bool				blocking;


	public:
		Socket(Type socketType, SOCKET mySd = INVALID_SOCKET) :  type(socketType), sd(mySd), blocking(true) {}

		//lint -e{1551} (Warning -- Function may throw exception '...' in destructor 'UDP::Socket::~Socket(void)')
		~Socket() {Close();}
		bool Open(uint16 portNumber = 0, uint32 interfaceAddress = 0);	//interfaceAddress is the address of the interface on this machine.  0 means any
		void Close()
		{
			if (sd != INVALID_SOCKET) 
				closesocket(sd);

			sd = INVALID_SOCKET;
		}

		uint16 GetPortNumber() const;
		Type GetType() const {return type;}

		bool SetToBlock(bool block)
		{
			bool success = true;
#ifdef _MSC_VER
			unsigned long nonBlock = block ? 0 : 1;
			int rc = ioctlsocket(sd, FIONBIO, &nonBlock);
			success = (rc == 0) ? true : false;
#else
			int opts = fcntl(sd, F_GETFL);
			if (opts < 0)
				return false;

			if (block)
				opts &= ~O_NONBLOCK;
			else
				opts |= O_NONBLOCK;
				
			success = (fcntl(sd, F_SETFL, opts) == 0) ? true : false;
#endif
			if (success)
				blocking = block;

			return success;
		}

		bool Blocking() const {return blocking;}

		SOCKET SocketDescriptor() const {return sd;}
	};
}

#endif

