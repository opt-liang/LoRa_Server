/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef TCP_SOCKET_HPP
#define TCP_SOCKET_HPP

#include "IpSocket.hpp"
#include "General.h"
#include <memory.h>
#include <string>

namespace TCP
{
	class Socket : public IP::Socket
	{
	private:
		sockaddr_in		partnerAddress;
		bool			connected;

	public:
		Socket() : IP::Socket(tcp), connected(false)
		{SetInvalid(partnerAddress);}

		Socket(SOCKET mySd, sockaddr_in const& myPartnerAddress)
			:IP::Socket(tcp, mySd), partnerAddress(myPartnerAddress), connected(true)
		{}

		virtual ~Socket() 
		{
			if (Connected())
				Disconnect();
		}

		bool Connect(bool waitForConnection, sockaddr_in const& myPartnerAddress)	//active connection is done by RefreshConnections()
		{
			partnerAddress = myPartnerAddress;

			if (SocketDescriptor() == INVALID_SOCKET)
			{
				if (!Reopen())
					return false;
			}

			if (waitForConnection)
				Connect();

			return true;
		}

private:
		bool Connect()	//Open must be called before connect - the alternative to calling this function is calling ConnectionManager::RefreshConnections()
		{
			if (SocketDescriptor() == INVALID_SOCKET)
			{
				if (!Reopen())
					return false;
			}

			return Connect(SocketDescriptor(), partnerAddress);
		}

public:
		void Disconnect();

		int Receive(uint8 receivedData[], uint16 maxMessage) const;	//return number of bytes (<0 is an error)
		bool Send(uint8 const data[], uint16 length); //returns false on error

		sockaddr_in const& PartnerAddress() const {return partnerAddress;}

		bool Connected() const {return connected;}	//returns true if socket is connected
		bool IsNowConnected(bool myConnected, IP::SocketSet& socketSet);		//called when the socket has been connected outside the class
		bool PartnerAddressKnown() const {return IsValidPort(partnerAddress);}

		virtual void SendKeepAliveData() {}


		static bool Connect(SOCKET const& socketDescriptor, sockaddr_in const& partnerAddress)
		{
			int connectResult = connect(socketDescriptor, (sockaddr const*) &partnerAddress, sizeof(partnerAddress));
			bool connected = (connectResult == 0) ? true : false;

			if (!connected)
			{
				int errorNo = IP::Error::ReadLastError();

				if (errorNo == IP::Error::alreadyConnected)	//already connected
					connected = true;

				else if (errorNo != IP::Error::wouldBlock)
					IP::lastError.Set("TCP connect failed", errorNo);
			}
			return connected;
		}
	};


	class ServerSocket : public Socket
	{
	private:
		unsigned const		maxWaitingConnections;

	public:
		ServerSocket(unsigned myMaxWaitingConnections = 20)
			: maxWaitingConnections(myMaxWaitingConnections)
		{}

		virtual ~ServerSocket() {}

		bool Open(uint16 portNumber)	//portNumber is the port on this machine
		{
			if (!Socket::Open(portNumber))
				return false;

			if (listen(SocketDescriptor(), maxWaitingConnections) == SOCKET_ERROR)
			{
				IP::lastError.Set("Call to listen failed");
				return false;
			}
			return true;
		}

		bool Open(uint16 portNumber, sockaddr_in const& interfaceAddress)	//interfaceAddress is the address of the interface on this machine.  0 means any
		{
			if (!Socket::Open(portNumber, interfaceAddress))
				return false;

			if (listen(SocketDescriptor(), maxWaitingConnections) == SOCKET_ERROR)
			{
				IP::lastError.Set("Call to listen failed");
				return false;
			}
			return true;
		}


		SOCKET WaitForConnection(sockaddr_in& connnectingPort) const	//returns INVALID_SOCKET on error.  Writes newly connected port to connectingPort on success
		{
			SocketAddressLengthType addressLength = sizeof(connnectingPort);

			SOCKET newSocketDescriptor = accept(SocketDescriptor(), (sockaddr*) &connnectingPort, &addressLength);
			if (newSocketDescriptor == INVALID_SOCKET)
				IP::lastError.Set("Error return from accept");

			return newSocketDescriptor;
		}
	};


	class ProtocolAwareSocket : public TCP::Socket
	{
		//calls to this function will return complete protocol messages
	protected:
		uint16 const		maxMessageLength;
		uint8* const		rxBuffer;
		uint8 const* const	rxBufferEnd;	//must be declared after data

		uint8*				receiveEnd;		//one past end of received bytes

		uint8*				protocolCurrentMessageStart;	//start of current protocol message
		uint8*				protocolCurrentMessageEnd;
		/*byte past the end of the current protocol message, or zero if none held.  
		Set by virtual function FindProtocolMessageEnd().. May be one past receiveEnd, 
		if protocol message end on a receive boundary */

	public:
		ProtocolAwareSocket(uint16 myMaxMessageLength)
			: maxMessageLength(myMaxMessageLength),
			rxBuffer(new uint8[2 * myMaxMessageLength]), 
			rxBufferEnd(rxBuffer + 2 * myMaxMessageLength),

			receiveEnd(rxBuffer),
			protocolCurrentMessageStart(0),
			protocolCurrentMessageEnd(0)
		{}

		ProtocolAwareSocket(uint16 myMaxMessageLength, SOCKET mySd, sockaddr_in const& myPartnerAddress)
			: TCP::Socket(mySd, myPartnerAddress),
			maxMessageLength(myMaxMessageLength),
			rxBuffer(new uint8[2 * myMaxMessageLength]), 
			rxBufferEnd(rxBuffer + 2 * myMaxMessageLength),

			receiveEnd(rxBuffer),
			protocolCurrentMessageStart(0),
			protocolCurrentMessageEnd(0)
		{}

	private:
		ProtocolAwareSocket(ProtocolAwareSocket const& other);	// not expected to be used or defined

	public:
		virtual ~ProtocolAwareSocket() {delete [] rxBuffer;}

		int Receive(uint8 output[], uint16 maxBytes)
		{
			/*Receive will attempt to write one application protocol message into output.  
			If the object's internal buffer does not already 
			hold a protocol message, the object will call the TCP::Socket::Receive function.  This may
			cause the thread to block (depending on whether SetToBlock(..) has been called.  
			ProtocolMessageWaiting() message waiting can be called to find whether a blocking 
			socket is guarranteed to return immediately

			If a socket error has occurred IP::Socket::error will be returned
			If the socket is now disconnected, IP::Socket::disconnected will be returned
			If no new protocol message is available, zero will be returned

			If askForMoreBytesFromBasicSocket is true, the function will call TCP::Socket::Receive if no full protocol message is available
			*/

			if (!ProtocolMessageWaiting() && Connected())	//ie if protocolNextMessageStart == 0
			{
				int bytesReceived = TCP::Socket::Receive(receiveEnd, rxBufferEnd - receiveEnd);

				if (bytesReceived == 0)
				{
					if (Blocking())
					{
						Disconnect();
						return IP::Socket::disconnected;
					}
					else
						return 0;
				}
				else if (bytesReceived == IP::Socket::disconnected)
					return IP::Socket::disconnected;
				else if (bytesReceived < 0)
					return IP::Socket::error;	// either zero (if blocking this indicates that the connection is closed) or -ve, indicating an error

				// bytesReceived is greater than 0
				uint8* receiveStart = receiveEnd;
				receiveEnd += bytesReceived;
				
				if (!protocolCurrentMessageStart)
					protocolCurrentMessageStart = FindCurrentProtocolMessageStart(receiveStart);

				protocolCurrentMessageEnd = FindCurrentProtocolMessageEnd();
			}

			int outputBytes = 0;

			if (ProtocolMessageWaiting())
			{
				// a full message is available - copy it out
				outputBytes = protocolCurrentMessageEnd - protocolCurrentMessageStart + InterMessageDelimiterLength();	//add separator

				if (outputBytes > maxBytes)
					outputBytes = maxBytes;	// the message has overflowed the output buffer - discard its end

				memcpy(output, protocolCurrentMessageStart, outputBytes);

				//update pointers for next message
				protocolCurrentMessageStart = FindCurrentProtocolMessageStart(protocolCurrentMessageEnd);

				protocolCurrentMessageEnd = FindCurrentProtocolMessageEnd();
			}

			//move unread message data if necessary
			if (!protocolCurrentMessageStart)	//buffer is empty
			{
				receiveEnd = rxBuffer;
			}
			else if (rxBufferEnd < receiveEnd + maxMessageLength) 	// no space at end of buffer - copy the waiting data to front
			{
				int bytesToMove = receiveEnd - protocolCurrentMessageStart + InterMessageDelimiterLength();
				//InterMessageDelimiterLength() ensures the delimiter (if present) is copied although it may not be present

				int distanceToMove = protocolCurrentMessageStart - rxBuffer;
				memmove(rxBuffer, protocolCurrentMessageStart, bytesToMove); //memmove because source and destination may overlap
				protocolCurrentMessageStart = rxBuffer;
				receiveEnd -= distanceToMove;

				if (protocolCurrentMessageEnd)
					protocolCurrentMessageEnd -= distanceToMove;
			}
			return outputBytes;
		}

		bool ProtocolMessageWaiting() const {return protocolCurrentMessageStart && protocolCurrentMessageEnd;}

	private:
		virtual uint8* FindCurrentProtocolMessageStart(uint8 const start[]) const = 0;
		//finds first byte of next protocol message or returns 0
		virtual uint8* FindCurrentProtocolMessageEnd() const = 0;
		//find one past final byte of current protocol message or returns 0

		virtual uint InterMessageDelimiterLength() const = 0;

		bool Consistent() const
		{
			if (receiveEnd < rxBuffer)
				goto fail;

			if (receiveEnd > rxBufferEnd)
				goto fail;

			if (protocolCurrentMessageStart)
			{
				if (protocolCurrentMessageStart < rxBuffer)
					goto fail;

				if (protocolCurrentMessageStart > receiveEnd)
					goto fail;
			}

			if (protocolCurrentMessageEnd && !protocolCurrentMessageStart)
				goto fail;

			if (protocolCurrentMessageEnd)
			{
				if (!protocolCurrentMessageStart)
					goto fail;

				if (protocolCurrentMessageStart >= protocolCurrentMessageEnd)
					goto fail;

				if (protocolCurrentMessageEnd < rxBuffer)
					goto fail;

				if (protocolCurrentMessageEnd > receiveEnd)
					goto fail;

			}

			return true;
fail:
			return false;
		}
	};
}

#endif
