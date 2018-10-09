/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef TCP_TEXT_SOCKET_CONNECTION_MANAGER_HPP
#define TCP_TEXT_SOCKET_CONNECTION_MANAGER_HPP

#include "General.h"
#include "TcpTextSocket.hpp"
#include "TcpConnectionManager.hpp"

namespace TCP
{
	class TextConnectionManager : public ConnectionManager
	{
	private:
		const uint16		maxMessageLength;

	public:
		TextConnectionManager(uint32 myListTickPeriod_ms, uint16 myMaxMessageLength, bool roundRobin) 
			: ::TCP::ConnectionManager(myListTickPeriod_ms, roundRobin), 
			maxMessageLength(myMaxMessageLength)
		{}

	private:
		//redefined virtual function
		virtual Socket* CreateSocket();
		virtual Socket* CreateSocket(SOCKET socketDescriptor, sockaddr_in const& remotePort);
	};
}
#endif

