/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef IP_SOCKET_HPP
#define IP_SOCKET_HPP

#include "General.h"
#include "Ip.h"
#include <string>
#include <limits.h>
#include <memory.h>

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
	const uint16 maxHostNameLength = 256;
	const uint16 maxReceivedMessage = 1024;
	const uint32 noErrorNumber = ~uint32(0);	//all ones
	extern const sockaddr_in nullAddress;

	bool ReadAddressText(char const text[], uint32& output); //converts dotted decimal into integer
	bool ReadAddressText(char const text[], sockaddr_in& output); //converts dotted decimal into structure
	inline bool ReadAddressText(std::string const& text, sockaddr_in& output) {return ReadAddressText(text.c_str(), output);}//converts dotted decimal into structure

	bool IsOnThisMachine(sockaddr_in const& address);

	bool Initialise();	//MUST be called at startup.  Automatically called by IP:::Socket::Open()

	inline sockaddr_in const& NullAddress() {return nullAddress;}	//only exists to keep lint and g++ happy

	class Error
	{
	public:
#ifdef _MSC_VER
		static const int wouldBlock = WSAEWOULDBLOCK;
		static const int connectionAborted = WSAECONNABORTED;
		static const int connectionReset = WSAECONNRESET;
		static const int alreadyConnected = WSAEISCONN;
		static const int badFile = EBADF;
		static const int notASocket = WSAENOTSOCK;
#else
		static const uint wouldBlock = EWOULDBLOCK;
		static const int connectionAborted = ECONNABORTED;
		static const int connectionReset = ECONNRESET;
		static const int alreadyConnected = EISCONN;
		static const int badFile = EBADF;
		static const int notASocket = EBADF;	//EBADF is used for both badFile and not a socket
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
	public:
		static const int error = -1;
		static const int immediateReturn = -2;
		static const int disconnected = immediateReturn;	//only relevant to tcp sockets
		enum Type {tcp, udp};

	private:
		const Type		type;
		SOCKET			sd;
		bool			blocking;
		SocketSet*		socketSet;	//pointer to socket set of which this is a member
		uint16			localPortNumber;
		sockaddr_in		interfaceAddress; 	//interfaceAddress is the address of the interface on this machine.  0 means any

	public:
		Socket(Type socketType, SOCKET mySd = INVALID_SOCKET) :  type(socketType), sd(mySd), blocking(true), socketSet(0), localPortNumber(0)
		{SetInvalid(interfaceAddress);}

		//lint -e{1551} (Warning -- Function may throw exception '...' in destructor 'UDP::Socket::~Socket(void)')
		virtual ~Socket() {Close();}
		bool Open(uint16 newPortNumber, sockaddr_in const& interfaceAddress);
		bool Open(uint16 newPortNumber = 0);
		void Close();
		sockaddr_in GetLocalAddress() const;
		uint16 GetPortNumber() const;
		Type GetType() const {return type;}
		sockaddr_in const& InterfaceAddress() const {return interfaceAddress;}

		bool SetToBlock(bool block)
		{
#ifdef _MSC_VER
			unsigned long nonBlock = (block ? 0 : 1);
			//lint -e{569} (Warning -- Loss of information (arg. no. 2) (32 bits to 31 bits))
			int rc = ioctlsocket(sd, FIONBIO, &nonBlock);
			bool success = (rc == 0) ? true : false;
#else
			int opts = fcntl(sd, F_GETFL);
			if (opts < 0)
				return false;

			if (block)
				opts &= ~O_NONBLOCK;
			else
				opts |= O_NONBLOCK;
				
			bool success = (fcntl(sd, F_SETFL, opts) == 0) ? true : false;
#endif
			if (success)
				blocking = block;

			return success;
		}

		bool Blocking() const {return blocking;}
		bool InSocketSet() const {return socketSet ? true : false;}
		bool AddToSocketSet(SocketSet& set);
		void RemoveFromSocketSet();
		bool SetActiveInSocketSet(bool active);

		SOCKET SocketDescriptor() const {return sd;}
		void SetSocketDescriptor(SOCKET newSd) {sd = newSd;}
		bool Validate() const;	//false indicates an error

		//static functions - for when the object must not be locked
		static SOCKET Reopen(Type socketType, sockaddr_in const& localInterfaceAddress, uint16 localPortNumber);
		static void Close(SOCKET socketDescriptor);

	protected:
		bool Reopen();
	};
}

#endif

