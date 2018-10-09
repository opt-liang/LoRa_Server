/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "JsonReceiveNS.hpp"
#include "JsonParser.hpp"
#include "JsonCommand.hpp"
#include "ExceptionClass.hpp"
#include "Utilities.hpp"
#include "LoRa.hpp"
#include "LoRaRegion.hpp"
#include "GlobalDataNS.hpp"
#include "CommandParserNS.hpp"
#include "MessageAddress.hpp"

#include <sstream>
#include <limits.h>


namespace JSON
{
	namespace Receive
	{
		void JoinAcceptMessage(EuiType moteEui, char const receivedText[]);
		void JoinCompleteMessage(EuiType moteEui, char const receivedText[]);

		void GatewayDataMessage(char const receivedText[], uint64 thisServerReceiveTime_ms, MessageAddress const& source, EuiType const& gatewayEui);
		void GatewayStatus(char const receivedText[], uint64 thisServerReceiveTime_ms, EuiType const& gatewayEui);
	}
}


void JSON::Receive::Top(char const receivedText[], uint64 thisServerReceiveTime_ms, MessageAddress const& source, EuiType const& gatewayEui)
{
	try
	{
		if (Debug::log.Print(Debug::verbose))
			Debug::Write(std::string("JSON msg rx ") + receivedText);

		//JSON format
		JSON::Parser parser(receivedText);
		//Find location of packet received from mote
		char const* idText;	//pointer to first character of id
		//valueText is pointer to first character of value text

		//lint --e{720} Info -- Boolean test of assignment
		while (idText = parser.FindName())	//find next object name
		{
			char const* valueText = parser.FindValue();	//find next object value

			if (valueText == 0)
				throw MessageRejectException(receivedText, std::string("Unable to read ") + idText + " value");

			if (Parser::Match(idText,"rxpk"))
			{
				if (*valueText == '[')
				{
					//is array
					JSON::Parser arrayParser(valueText+1, false);

					while (valueText = arrayParser.FindValue(true))
					{
						GatewayDataMessage(valueText, thisServerReceiveTime_ms, source, gatewayEui);
					}

					parser.Update(arrayParser);
				}
				else  // is not an array
				{
					GatewayDataMessage(valueText, thisServerReceiveTime_ms, source, gatewayEui);
				}
			}

			else if (Parser::Match(idText,"stat"))
				GatewayStatus(valueText, thisServerReceiveTime_ms, gatewayEui);
		}
	}
	catch (JSON::MessageRejectException const& e)
	{
		if (Debug::log.Print(Debug::verbose))
		{
			std::stringstream text;

			text << "Message rejected - " << e.Explanation();
					
			if (e.NumberSet())
				text << e.Number();

			text << e.Message() << std::endl;

			Debug::Write(text);
		}
	}
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

		CommandParserNS parser;
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


void JSON::Receive::GatewayDataMessage(char const receivedText[], uint64 thisServerReceiveTime_ms, MessageAddress const& source, EuiType const& gatewayEui)
{
	if ((gatewayEui == invalidEui) || (gatewayEui == nullEui))
		return;	//invalid (null) gateway

	char const* idText;
	ValidValueUint16 dataSize;
	JSON::Parser parser(receivedText);
	ValidValueBool frameCrc;
	LoRa::ValidModulationType modulation;

	LoRa::Region region = Global::gatewayList.GetRegion(gatewayEui);
	if (!LoRa::IsValidRegion(region))
	{
		if (Debug::Print(Debug::verbose))
		{
			std::stringstream text;

			text << "Frame received from Gateway " << AddressText(gatewayEui) << ", which belongs to an unknown region";
			Debug::Write(text);
		}
		return;
	}

	LoRa::ReceivedFrame receivedFrame(thisServerReceiveTime_ms);

	receivedFrame.GatewayEui(gatewayEui);
	receivedFrame.GatewayAddress(source);
	receivedFrame.GatewayRegion(region);

	//lint --e{720} Info -- Boolean test of assignment
	while (idText = parser.FindName())
	{
		char const* valueText = parser.FindValue();

		if (valueText == 0)
		{
			std::string message = "Unable to read ";
			message += idText;
			message += " value";
			throw MessageRejectException(receivedText, message);
		}

		if (Parser::Match(idText,"time"))
		{
			receivedFrame.gatewayReceiveTime = valueText;
			receivedFrame.gatewayReceiveTime.Accurate(true);
		}
		else if (Parser::Match(idText,"tmst"))
		{
			receivedFrame.gatewayReceiveTimestamp_us = ReadUnsignedInteger(valueText);
		}
		else if (Parser::Match(idText,"stat"))
		{
			int crcStatus = ReadSignedInteger(valueText);

			if (crcStatus != 1)
				throw MessageRejectException(receivedText, "CRC check failed");

			frameCrc = true;
		}
		else if (Parser::Match(idText,"size"))
		{
			dataSize = ReadUnsignedInteger(valueText);
		}
		else if (Parser::Match(idText,"rssi"))
		{
			signed signalStrength_dBm = ReadSignedInteger(valueText);

			if (signalStrength_dBm > SHRT_MIN)
				receivedFrame.signalStrength_cBm = signalStrength_dBm * 10;	//convert to cBm
			else
				throw MessageRejectException(receivedText, "Invalid rssi value");
		}
		else if (Parser::Match(idText,"chan"))
		{
			uint32 channel = ReadUnsignedInteger(valueText);

			if (channel != ~uint32(0))
				receivedFrame.channel = uint8(channel);
		}
		else if (Parser::Match(idText,"freq"))
		{
			uint32 frequency_Hz = ReadUnsignedFixPointNumberAsInteger(valueText,6);

			if (frequency_Hz == ~uint32(0))
				throw MessageRejectException(receivedText, "Invalid freq value");

			receivedFrame.frequency_Hz = frequency_Hz;
		}
		else if (Parser::Match(idText,"rfch"))
		{
			uint8 rfChain = uint8(ReadUnsignedInteger(valueText));

			receivedFrame.rfChain = rfChain;
		}
		else if (Parser::Match(idText,"codr"))
		{
			LoRa::CodeRate codeRate = valueText;

			if (!codeRate.Valid())
				throw MessageRejectException(receivedText, "Unable to read code rate value");

			receivedFrame.codeRate = codeRate;
		}
		else if (Parser::Match(idText,"datr"))
		{
			LoRa::DataRate dataRate = valueText;
			if (!dataRate.Valid())
				throw MessageRejectException(receivedText, "Unable to read datarate value");

			receivedFrame.dataRate = dataRate;
		}
		else if (Parser::Match(idText,"lsnr"))
		{
			sint16 signalToNoiseRatio_cB = sint16(ReadSignedFixPointNumberAsInteger(valueText, 1));

			if (signalToNoiseRatio_cB > SHRT_MIN)
				receivedFrame.signalToNoiseRatio_cB = signalToNoiseRatio_cB;
			else
				throw MessageRejectException(receivedText, "Invalid lsnr value");
		}
		else if (Parser::Match(idText,"modu"))
		{
			if (valueText[0] == 'L')
				modulation = LoRa::loRaMod;
			else if (valueText[0] == 'F')
				modulation = LoRa::fskMod;
			else
				throw MessageRejectException(receivedText, "Invalid modu value");
		}
		else if (Parser::Match(idText,"data"))
		{
			sint16 length = ConvertBase64TextToBinaryArray(valueText, receivedFrame.DataNonConst(), LoRa::maxFrameLength);

			if (length >= 0)
			{
				receivedFrame.SetLength(length);

				if (Debug::Print(Debug::verbose))
				{
					std::stringstream text;
					text << ConvertBinaryArrayToHexTextBlock("Received frame", receivedFrame.Data(), receivedFrame.Length());
					Debug::Write(text);
				}

				if (receivedFrame.Length() == 0)
					throw MessageRejectException(receivedText, std::string("Data invalid :") +  ReadValue(valueText));
			}
		}
	}

	if (modulation.Valid() && receivedFrame.dataRate.Valid() && modulation != receivedFrame.dataRate.modulation)
		throw MessageRejectException(receivedText, "Conflict between modu and datr");

	if (dataSize.Valid() && dataSize != receivedFrame.Length())
		throw MessageRejectException(receivedText, "Data size does not match bytes received");

	if (!receivedFrame.frequency_Hz.Valid())
		throw MessageRejectException(receivedText, "Received frame meta-data does not contain a valid frequency");

	if (!receivedFrame.Valid())
		throw MessageRejectException(receivedText, "Received frame contains insufficient meta-data");

	try
	{
		if (frameCrc.Valid() && frameCrc.Value())
			receivedFrame.Receive();
	}
	catch (LoRa::ReceivedFrame::MessageRejectException const& e)
	{
		if (Debug::Print(Debug::verbose))
		{
			std::stringstream text;

			text << "LoRa::ReceivedFrame::MessageRejectException : "
				<< e.Explanation();

			if (e.NumberSet())
				text << " Error number " << e.Number();

			Debug::Write(text);
		}
	}
}


void JSON::Receive::GatewayStatus(char const receivedText[], uint64 thisServerReceiveTime_ms, EuiType const& gatewayEui)
{
	GatewayStatusType status;
	char const* idText;
	JSON::Parser parser(receivedText);
	ValidValueFloat upStreamDatagramsAcknowledgedRatio;
	ValidValueUint32 upstreamPacketsReceived;

	//lint --e{720} Info -- Boolean test of assignment
	while (idText = parser.FindName())
	{
		char const* valueText = parser.FindValue();

		if (valueText == 0)
		{
			std::string message = "Unable to read ";
			message += idText;
			message += " value";
			throw MessageRejectException(receivedText, message);
		}

		if (Parser::Match(idText,"time"))
		{
			status.time = valueText;
		}
		else if (Parser::Match(idText,"lati"))
		{
			double value;

			if (ReadRealNumber(valueText, value))
				status.position.latitude = value;
		}
		else if (Parser::Match(idText,"long"))
		{
			double value;

			if (ReadRealNumber(valueText, value))
				status.position.longitude = value;
		}
		else if (Parser::Match(idText,"alti"))
		{
			double value;
			
			if (ReadRealNumber(valueText, value))
				status.position.altitude = value;
		}
		else if (Parser::Match(idText,"rxnb"))
		{
			uint32 value = ReadUnsignedInteger(valueText);

			if (value < ~uint32(0))	//valid
			{
				upstreamPacketsReceived = value;
				status.count.upstreamPacketsReceived = value;
			}
		}
		else if (Parser::Match(idText,"rxok"))
		{
			uint32 value = ReadUnsignedInteger(valueText);

			if (value < ~uint32(0))	//valid
				status.count.upstreamGoodPacketsReceived = value;
		}
		else if (Parser::Match(idText,"rxfw"))
		{
			uint32 value = ReadUnsignedInteger(valueText);

			if (value < ~uint32(0))	//valid
				status.count.upstreamPacketsForwarded = value;
		}
		else if (Parser::Match(idText,"ackr"))
		{
			double value;
			
			if (ReadRealNumber(valueText, value))
				upStreamDatagramsAcknowledgedRatio = float(value / 100.0);
		}
		else if (Parser::Match(idText,"dwnb"))
		{
			uint32 value = ReadUnsignedInteger(valueText);

			if (value < ~uint32(0))	//valid
				status.count.downstreamDatagramsReceived = value;
		}
		else if (Parser::Match(idText,"txnb"))
		{
			uint32 value = ReadUnsignedInteger(valueText);

			if (value <~uint32(0))	//valid
				status.count.packetsTransmitted = value;
		}
	}

	if (upstreamPacketsReceived.Valid() && upStreamDatagramsAcknowledgedRatio.Valid())
		status.count.upStreamDatagramsAcknowledged += uint32(upstreamPacketsReceived.Value() * upStreamDatagramsAcknowledgedRatio.Value() + 0.5);
	//0.5 added to give correct integer after any rounding errors

	Global::gatewayList.StatusReceived(gatewayEui, thisServerReceiveTime_ms, status);
}


void JSON::Receive::AppDataMessage(EuiType moteEui, ValidValueUint32 const& sequenceNumber, ValidValueUint16 const& token, 
			LoRa::FrameApplicationData const& payload, MoteTransmitRecord const& /*transmitRecord*/, GatewayReceiveList const& /*gatewayReceiveList*/, 
			MessageAddress const& /*source*/, bool up)
{
	if (up)
		throw JSON::MessageRejectException("Incorrect payload source");

	if (!sequenceNumber.Valid())
		throw JSON::MessageRejectException("Invalid sequence number");

	if (!Global::moteList.DownstreamApplicationDataReceived(moteEui, sequenceNumber, payload, token))
	{
		int sendRejectMessage;
	}
}


void JSON::Receive::MacCommandMessage(EuiType moteEui, ValidValueUint16 const& token, LoRa::OptionRecord command)
{
	Mote* mote = Global::moteList.GetById(moteEui);

	if (!mote)
		return;

	mote->SendMacCommand(command, token);
	mote->Unlock();
}


void JSON::Receive::MoteMessage(char const receivedText[], MessageAddress const& source)
{
	char const* idText;
	JSON::Parser parser(receivedText);
	bool queueLengthQuery = false;
	bool seqNoRequest = false;
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

		else if (JSON::Parser::Match(idText, "app"))
		{
			sint8 readValue  = ReadBoolean(valueText);
			
			if (readValue < 0)
				continue;

			app = (readValue == 1 ? true : false);
		}

		else if (JSON::Parser::Match(idText,"qlenquery"))
			queueLengthQuery = true;

		else if (JSON::Parser::Match(idText, "seqnoreq"))
			seqNoRequest = true;
	}

	if (!moteEui.Valid())
		throw JSON::MessageRejectException(receivedText, "Unable to read mote EUI");

	::Mote* mote = Global::moteList.GetById(moteEui);	//Mote is locked
	if (!mote)
		throw JSON::MessageRejectException(receivedText, "Message received for unknown mote");

	if (seqNoRequest)
		mote->FrameSequenceNumberGrantRequestReceived(source);

	//all objects after this point require app.Valid()
	if (app.Valid())
	{
		if (queueLengthQuery)
			mote->QueueLengthQueryReceived(source.TcpConnectionId(), app);
	}
	mote->Unlock();
}


void JSON::Receive::JoinMessage(char const receivedText[])
{
	char const* idText;
	JSON::Parser parser(receivedText);
	ValueWithValidity<EuiType> moteEui;
	bool joinAccept = false;
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

		if (Parser::Match(idText,"moteeui"))
		{
			moteEui = ReadEui(valueText);

			if (moteEui == invalidEui)
				throw JSON::MessageRejectException(receivedText,"moteeui coding error");
		}

		else if (JSON::Parser::Match(idText,"accept"))
		{
			joinAccept = true;
			joinMessage = valueText;
		}
		else if (JSON::Parser::Match(idText,"complete"))
		{
			joinAccept = false;
			joinMessage = valueText;
		}
	}

	if (!moteEui.Valid())
		throw JSON::MessageRejectException(receivedText, "Unable to read moteEui from join message");

	if (!joinMessage)
		throw JSON::MessageRejectException(receivedText, "Unable to read content of join message");

	if (joinAccept)
		Receive::JoinAcceptMessage(moteEui, joinMessage);
	else
		Receive::JoinCompleteMessage(moteEui, joinMessage);
}


void JSON::Receive::JoinAcceptMessage(EuiType moteEui, char const receivedText[])
{
	bool accept = false;

	int readValue = JSON::Receive::ReadBooleanValue(receivedText);

	if (readValue >= 0)
		accept = bool(readValue != 0);
	else
		throw JSON::MessageRejectException(receivedText,"ReceiveJoinAcceptMessage() unable to read value");

	Global::joinController.ReceivedAccept(moteEui, accept);
}


void JSON::Receive::JoinCompleteMessage(EuiType moteEui, char const receivedText[])	//receivedText must be null terminated
{
	char const* idText;
	JSON::Parser parser(receivedText);
	LoRa::Frame frame;
	ValueWithValidity<LoRa::CypherKey> networkKey;

	//lint --e{720} Info -- Boolean test of assignment
	while (idText = parser.FindName())
	{
		char const* valueText = parser.FindValue();

		if (valueText == 0)
		{
			std::string message = "Unable to read ";
			message += idText;
			message += " value";
			throw MessageRejectException(receivedText, message);
		}

		else if (Parser::Match(idText,"networkkey"))
		{
			uint8 keyArray[LoRa::cypherKeyBytes];
			
			if (!ConvertFixedLengthHexTextToBinaryArray(valueText, keyArray, LoRa::cypherKeyBytes, true))
				throw MessageRejectException(receivedText,"Join complete network key coding error");

			networkKey = keyArray;
		}
		else if (JSON::Parser::Match(idText,"frame"))
		{
			std::string frameText = ReadValue(valueText);

			sint16 bytes = ConvertBase64TextToBinaryArray(frameText.c_str(), frame.DataNonConst(), LoRa::maxFrameLength);

			if (bytes <= 0)
				throw MessageRejectException(receivedText,"Join complete frame coding error");

			frame.SetLength(bytes);
		}
	}

	Global::joinController.ReceivedComplete(moteEui, frame, networkKey);
}

