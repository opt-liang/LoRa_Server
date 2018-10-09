/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "CommandParser.hpp"
#include "TcpConnectionManager.hpp"
#include "GlobalData.hpp"

void CommandParser::ParseConnectionListCommand()
{
	std::stringstream text;

	text <<
		"Port " << Global::tcpConnectionAddressController.AdvertisedLocalPort() << std::endl <<
		Global::tcpConnectionManager.Print();

	Write(text);
}

void CommandParser::ParseConnectionTestCommand()
{
	JSON::String request;

	request.Open();
	request.OpenStructuredObject("ip", false);
	request.AddTextValue("pingrequest", "", false);
	request.Close();	//ip
	request.Close();	//top

	Global::tcpConnectionManager.Send(request);
}

