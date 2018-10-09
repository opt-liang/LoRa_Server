/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "JsonReceive.hpp"
#include "JsonParser.hpp"
#include "JsonString.hpp"
#include "JsonCommand.hpp"
#include "GlobalDataCS.hpp"
#include "CommandParserCS.hpp"
#include "TransmissionRecord.hpp"
#include "MessageAddress.hpp"


namespace JSON
{
	namespace Receive
	{
		void MoteJoinMessage(char const receivedText[], EuiType moteEui, MessageAddress const& source);
	}
}

void JSON::Receive::AppDataMessage(EuiType moteEui, ValidValueUint32 const& sequenceNumber, ValidValueUint16 const& token,
			LoRa::FrameApplicationData const& payload, MoteTransmitRecord const& transmitRecord, GatewayReceiveList const& gatewayReceiveList, 
			MessageAddress const& source, bool up)
{
	if (!up)
		throw JSON::MessageRejectException("Received appdata message containg invalid direction");

	Global::moteList.ApplicationDataReceived(up, moteEui, sequenceNumber, token, payload, transmitRecord, gatewayReceiveList);
}


void JSON::Receive::CommandMessage(char const receivedText[], MessageAddress const& source)
{
	if (!Global::allowRemoteConfiguration && !source.ConnectionValid() && !source.IsLoopBack())
		return;	//if remote configuration not allowed and connection is not via TCP abort

	Global::commandLineInterface.RequestReceived(source);

	try
	{
		std::string commandText = ReadValue(receivedText);

		if (commandText.empty())
			return;

		CommandParserCS parser;
		parser.Parse(commandText);
	}

	catch (Exception const& e)
	{
		if (Debug::Print(e.Level()))
		{
			std::stringstream errorText;
				
			errorText << "Console exception : " << e.Filename() << ":" << e.Line() << ": " << e.Explanation();

			if (e.NumberSet())
				errorText << " " << e.Number();

			Debug::Write(errorText);
		}
	}
}


void JSON::Receive::MoteMessage(char const receivedText[], MessageAddress const& source)
{
	char const* idText;
	JSON::Parser parser(receivedText);
	bool resetDetected = false;
	bool ackRxDetected = false;
	char const* joinText = 0;
	bool queueLengthQuery = false;
	ValidValueUint16 queueLength;
	ValidValueUint16 sentToken;
	ValidValueBool app;
	Position position;

	ValidValueEuiType moteEui;

	//lint --e{720} Info -- Boolean test of assignment
	while (idText = parser.FindName())
	{
		char const* valueText = parser.FindValue();

		if (valueText == 0)
		{
			std::string message = "Unable to read ";
			message += idText;
			message += " value";
			throw JSON::MessageRejectException(receivedText, message);
		}

		else if (JSON::Parser::Match(idText, "eui"))
			moteEui = ReadUnsignedLongInteger(valueText, true, true);

		else if (JSON::Parser::Match(idText, "msgsent"))
			sentToken = ReadUnsignedInteger(valueText);

		else if (JSON::Parser::Match(idText, "app"))
		{
			sint8 readValue  = ReadBoolean(valueText);
			
			if (readValue < 0)
				continue;

			app = (readValue == 1 ? true : false);
		}

		else if (JSON::Parser::Match(idText,"ackrx"))
			ackRxDetected = true;

		else if (JSON::Parser::Match(idText,"qlenquery"))
			queueLengthQuery = true;

		else if (JSON::Parser::Match(idText,"qlen"))
			queueLength = ReadUnsignedInteger(valueText);

		else if (JSON::Parser::Match(idText,"join"))
			joinText = valueText;

		else if (JSON::Parser::Match(idText,"resetdetected"))
			resetDetected = true;

		else if(JSON::Parser::Match(idText,"posn"))
			position = ReadPosition(valueText);
	}

	if (!moteEui.Valid() || moteEui == invalidEui)
		throw JSON::MessageRejectException(receivedText, "Unable to read mote EUI");

	if (joinText)
		Receive::MoteJoinMessage(joinText, moteEui, source);

	if (position.Valid())
		Global::applicationDataOutput.PositionResultReceived(moteEui, position);

	if (sentToken.Valid())
		Global::moteList.AppDataTransmittedToGateway(moteEui, sentToken);

	if (queueLength.Valid())
		Global::moteList.QueueLengthReceived(moteEui, queueLength);
}

void JSON::Receive::MoteJoinMessage(char const receivedText[], EuiType moteEui, MessageAddress const& source)
{
	char const* idText;
	JSON::Parser parser(receivedText);
	ValidValueEuiType appEui;
	
	//lint --e{720} Info -- Boolean test of assignment
	while (idText = parser.FindName())
	{
		char const* valueText = parser.FindValue();

		if (valueText == 0)
		{
			std::string message = "Unable to read ";
			message += idText;
			message += " value";
			throw JSON::MessageRejectException(receivedText, message);
		}

		else if (JSON::Parser::Match(idText, "appeui"))
			appEui = ReadUnsignedLongInteger(valueText, true, true);
	}
	if (appEui.Valid() && appEui != invalidEui)
	{
		// join message
		Global::moteList.CreateMote(moteEui, appEui);
	}
}

