/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "JsonGenerate.hpp"

bool JSON::Send(MessageAddress const& destination, char const data[], uint16 length)
{
	if (Debug::Print(Debug::verbose))
	{
		std::stringstream printText;

		printText << "Transmitting JSON message to ";
		if (destination.ConnectionValid())
			printText << "TCP Connection " << destination.TcpConnectionId();
		else
			printText << AddressText(destination.Address());

		printText << "  ||" << data;
		Debug::Write(printText);
	}

	if (!destination.IsValid())
	{
		if (Debug::Print(Debug::monitor))
			Debug::Write("JSON::Send called with invalid destination");

		return false;
	}

	return SendPrivate(destination, data, length);
}

