/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef IP_H
#define IP_H

#include "General.h"

#ifdef _MSC_VER
	#include <WS2tcpip.h>
#else
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netdb.h>
	#include <unistd.h>
	#include <sys/ioctl.h> 
#endif

#include <memory.h>

inline bool IsValid(sockaddr_in const& in) {return in.sin_addr.s_addr != 0;}
inline void SetInvalid(sockaddr_in& in) {memzero(&in);}
inline bool IsValidPort(sockaddr_in const& in) {return IsValid(in) && in.sin_port != 0;}
bool IsLoopBack(sockaddr_in const& address);
inline int operator==(sockaddr_in const& r, sockaddr_in const& l) //compares addresses and protocol.  If both ports !=0, compares ports
{
	if (l.sin_addr.s_addr != r.sin_addr.s_addr) return 0;

	if ((l.sin_port != 0) && (r.sin_port != 0))
		if (l.sin_port != r.sin_port) return 0;

	if (l.sin_family != r.sin_family) return 0;

	return 1;
}


#endif

