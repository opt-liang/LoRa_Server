/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef TCP_TEXT_SOCKET_HPP
#define TCP_TEXT_SOCKET_HPP

#include "General.h"
#include "TcpSocket.hpp"
#include "TcpConnectionManager.hpp"

#include "TcpSocket.hpp"

namespace TCP
{
	class TextSocket : public ::TCP::ProtocolAwareSocket
	{
	public:
		TextSocket(uint16 myMaxMessageLength)
			: ::TCP::ProtocolAwareSocket(myMaxMessageLength)
		{}

		TextSocket(uint16 myMaxMessageLength, SOCKET mySd, sockaddr_in const& myPartnerAddress)
			:	::TCP::ProtocolAwareSocket(myMaxMessageLength, mySd, myPartnerAddress)
		{}

		void SendKeepAliveData()
		{
			//ASCII null symbol
			const uint8 data = 0;
			Send(&data, 1);
		}

	private:
		uint8* FindCurrentProtocolMessageStart(uint8 const start[]) const
		{
			if (!start)
				return 0;

			uint8 const* cptr = start;

			for (;cptr < receiveEnd; cptr++)
			{
				if (*cptr != '\0')
					return const_cast<uint8*>(cptr);
			}
			return 0;
		}

		uint8* FindCurrentProtocolMessageEnd() const	//first byte after current message
		{
			uint8 const* cptr = protocolCurrentMessageStart;

			if (!cptr)
				return 0;

			for (;cptr < receiveEnd; cptr++)
			{
				if (*cptr == '\0')
					return const_cast<uint8*>(cptr);
			}
			return 0;
		}

		uint InterMessageDelimiterLength() const {return 1;}	//the null character
	};
}
#endif
