/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "JsonReceive.hpp"
#include "JsonGenerate.hpp"
#include "JsonParser.hpp"
#include "JsonString.hpp"
#include "JsonCommand.hpp"
#include "JoinControllerAS.hpp"
#include "GlobalDataAS.hpp"
#include "CommandParserAS.hpp"
#include "TransmissionRecord.hpp"
#include "MessageAddress.hpp"


namespace JSON
{
	namespace Receive
	{
		void JoinRequestMessage(char const receivedText[]);
		void JoinDetailsMessage(EuiType moteEui, EuiType appEui, char const receivedText[]);
	}
}

void JSON::Receive::JoinMessage(char const receivedText[])
{
	char const* idText;
	JSON::Parser parser(receivedText, true);
	ValidValueEuiType appEui;
	ValidValueEuiType moteEui;
	bool request = false;
	char const* joinMessage = 0;

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

		if (JSON::Parser::Match(idText,"appeui"))
			appEui = ReadUnsignedLongInteger(valueText, true, true);

		else if (JSON::Parser::Match(idText,"moteeui"))
			moteEui = ReadUnsignedLongInteger(valueText, true, true);

		else if (JSON::Parser::Match(idText,"request"))
		{
			request = true;
			joinMessage = valueText;
		}

		else if (JSON::Parser::Match(idText,"details"))
		{
			request = false;
			joinMessage = valueText;
		}
	}

	if (!request && !appEui.Valid())
		throw JSON::MessageRejectException(receivedText, "Unable to read appEui from join message");

	if (!request && !moteEui.Valid())
		throw JSON::MessageRejectException(receivedText, "Unable to read moteEui from join message");

	if (!joinMessage)
		throw JSON::MessageRejectException(receivedText, "Unable to read content of join message");

	if (request)
		JoinRequestMessage(joinMessage);
	else
		JoinDetailsMessage(moteEui, appEui, joinMessage);
}


void JSON::Receive::JoinRequestMessage(char const receivedText[])
{
	char const* idText;
	JSON::Parser parser(receivedText, true);
	std::string frame;

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

		if (JSON::Parser::Match(idText,"frame"))
			frame = ReadValue(valueText);
	}


	if (frame.empty())
		throw JSON::MessageRejectException(receivedText, "Empty join request frame");

	uint8 rxData[LoRa::Frame::joinRequestBytes];
	sint16 bytes = ConvertBase64TextToBinaryArray(frame.c_str(), rxData, LoRa::Frame::joinRequestBytes);

	if (bytes < LoRa::Frame::joinRequestBytes)
		throw JSON::MessageRejectException(receivedText, "Unable to decode join request base64 data");

	Global::joinController.ReceivedRequest(rxData);
}

void JSON::Receive::JoinDetailsMessage(EuiType moteEui, EuiType appEui, char const receivedText[])
{
	char const* idText;
	JSON::Parser parser(receivedText);
	ValidValueUint32 moteAddress;
	ValidValueUint16 deviceNonce;

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

		else if (JSON::Parser::Match(idText,"moteaddr"))
			moteAddress = ReadUnsignedInteger(valueText, true, true);

		else if (JSON::Parser::Match(idText,"devicenonce"))
			deviceNonce = ReadUnsignedInteger(valueText);
	}

	if (!moteAddress.Valid() || 
		!deviceNonce.Valid())
		throw JSON::MessageRejectException(receivedText, "Unable to read join details message");

	Global::joinController.ReceivedDetails(moteEui, appEui, moteAddress, deviceNonce);
}

void JSON::Receive::AppDataMessage(EuiType moteEui, ValidValueUint32 const& sequenceNumber, ValidValueUint16 const& token, 
			LoRa::FrameApplicationData const& payload, 
			MoteTransmitRecord const& transmitRecord, GatewayReceiveList const& gatewayReceiveList, 
			MessageAddress const& source, bool up)
{
	if (up)
	{
		if (!sequenceNumber.Valid())
			throw JSON::MessageRejectException("Received upstream appdata message did not contain frame sequence number");

		if (!transmitRecord.Valid())
			throw JSON::MessageRejectException("Received upstream appdata message did not contain mote transmit information");

		if (gatewayReceiveList.IsEmpty())
			throw JSON::MessageRejectException("Received upstream appdata message did not contain gateway receive record");
	}

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

		CommandParserAS parser;
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
	bool queueLengthQuery = false;
	ValidValueUint32 sequenceNumber;
	ValidValueUint16 queueLength;
	ValidValueUint16 sentToken;
	ValidValueBool app;

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

		else if (JSON::Parser::Match(idText, "seqnogrant"))
			sequenceNumber = ReadUnsignedInteger(valueText);

		else if (JSON::Parser::Match(idText,"ackrx"))
			ackRxDetected = true;

		else if (JSON::Parser::Match(idText,"qlenquery"))
			queueLengthQuery = true;

		else if (JSON::Parser::Match(idText,"qlen"))
			queueLength = ReadUnsignedInteger(valueText);

		else if (JSON::Parser::Match(idText,"resetdetected"))
			resetDetected = true;
	}

	if (!moteEui.Valid())
		throw JSON::MessageRejectException(receivedText, "Unable to read mote EUI");

	::Mote* mote = Global::moteList.GetById(moteEui);	//Mote is locked

	if (!mote)
		throw JSON::MessageRejectException(receivedText, "Message received for unknown mote");

	if (resetDetected)
		mote->ResetDetected();

	if (sequenceNumber.Valid())
		mote->SequenceNumberGranted(sequenceNumber);

	//all objects after this point require app.Valid()
	if (app.Valid())
	{
		if (sentToken.Valid())
			mote->DataTransmittedToGateway(app, sentToken);

		if (ackRxDetected)
			mote->AcknowledgeReceived(app);

		if (queueLengthQuery)
			mote->QueueLengthQueryReceived(app);

		if (queueLength.Valid())
			mote->QueueLengthReceived(app, queueLength);
	}
	mote->Unlock();
}

