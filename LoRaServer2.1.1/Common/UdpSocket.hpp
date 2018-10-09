/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef UDP_SOCKET_HPP
#define UDP_SOCKET_HPP

#include "IpSocket.hpp"
#include "General.h"
#include <string>

namespace UDP
{
	class Socket : public IP::Socket
	{
	public:
		Socket() : IP::Socket(udp)
		{}

		virtual ~Socket() {}

		int Receive(sockaddr_in& source, uint8 receivedData[], uint16 maxMessage) const;	//return number of bytes (<0 is an error)
		bool Send(sockaddr_in const& destination, uint8 const data[], uint16 length) const; //returns false on error
		bool Send(sockaddr_in const& destination, std::string text) {return Send(destination, (uint8 const*) text.c_str(), (uint16) text.length() + 1);}
	};

}

#endif
