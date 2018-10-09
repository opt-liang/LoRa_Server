/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef JSON_FRAME_HPP
#define JSON_FRAME_HPP

#include "General.h"
#include "MessageAddress.hpp"

namespace JSON
{
	bool Send(MessageAddress const& destination, char const data[], uint16 length);
	inline bool Send(MessageAddress const& destination, std::string const& text)
	{return Send(destination, text.c_str(), static_cast<uint16>(text.length() + 1));}

	bool SendPrivate(MessageAddress const& destination, char const data[], uint16 length);	//called by Send and defined in each server
	bool SendPrivate(MessageAddress const& destination, uint8 const data[], uint16 length, bool useMessageProcotolSocket);	//defined only in Network Server
}

#endif

