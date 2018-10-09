/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "JsonReceive.hpp"
#include "JsonParser.hpp"
#include "Utilities.hpp"
#include "LoRa.hpp"
#include "TcpConnectionAddressController.hpp"
#include "GlobalData.hpp"

#include <string>
#include <sstream>

void JSON::Receive::IpControlMessage(char const receivedText[], MessageAddress const& source)
{
	JSON::Parser parser(receivedText);
	//Find location of packet received from mote
	char const* idText;	//pointer to first character of id
	//valueText is pointer to first character of value text

	if (!source.ConnectionValid())
		throw MessageRejectException(receivedText, "IpControlMessage Connection id is invalid");

	TCP::Connection::IdType connection = source.TcpConnectionId();

	//lint --e{720} Info -- Boolean test of assignment
	while (idText = parser.FindName())	//find next object name
	{
		char const* valueText = parser.FindValue();	//find next object value

		if (valueText == 0)
			throw MessageRejectException(receivedText, std::string("Unable to read ") + idText + " value");

		else if (Parser::Match(idText,"whichport"))
		{
			Global::tcpConnectionAddressController.QueryReceived(connection);
		}
		else if (Parser::Match(idText,"publishedport"))
		{
			uint32 port = ReadUnsignedInteger(valueText);

			if (port == ~uint32(0) || port > 0xFFFF)
				throw MessageRejectException(receivedText, "Unable to read IP published port");

			Global::tcpConnectionAddressController.ResponseReceived(connection, port);
		}
		else if (Parser::Match(idText, "pingrequest"))
		{
			sockaddr_in localAddress = Global::tcpConnectionManager.GetLocalAddress(connection);
			std::string addressText;

			if (IsValid(localAddress))
				addressText = AddressText(localAddress);

			JSON::String response;

			response.Open();
			response.OpenStructuredObject("ip", false);
			response.AddTextValue("pingresponse", addressText, false);
			response.Close();	//ip
			response.Close();	//top

			Global::tcpConnectionManager.Send(connection, response);
		}
		else if (Parser::Match(idText,"pingresponse"))
		{
			std::stringstream text;
			
			text << "Response rx connection " << connection << " " << ReadValue(valueText);
			Global::commandLineInterface.Write(text);
		}
	}
}

