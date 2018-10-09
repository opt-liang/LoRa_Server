/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef JSON_RECEIVE_NETWORK_SERVER_HPP
#define JSON_RECEIVE_NETWORK_SERVER_HPP

#include "General.h"
#include "LoRa.hpp"
#include "JsonReceive.hpp"

#include <string>

namespace JSON
{
	namespace Receive
	{
		void Top(char const receivedText[], uint64 thisServerReceiveTime_ms, MessageAddress const& source, EuiType const& gatewayEui);	//receivedText must be null terminated
	}
}

#endif

