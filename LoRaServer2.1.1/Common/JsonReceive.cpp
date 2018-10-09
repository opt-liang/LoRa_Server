/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "JsonReceive.hpp"
#include "JsonGenerate.hpp"
#include "JsonParser.hpp"
#include "JsonCommand.hpp"
#include "Utilities.hpp"
#include "LoRa.hpp"
#include "LoRaRegion.hpp"
#include "Position.hpp"

#include <string>
#include <sstream>

void JSON::Receive::Top(char const receivedText[], MessageAddress const& source, bool allowDataTraffic)
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

			else if (allowDataTraffic && Parser::Match(idText,"app"))
				Receive::AppMessage(valueText, source);

			else if (allowDataTraffic && Parser::Match(idText,"mote"))
				Receive::MoteMessage(valueText, source);

			else if (JSON::Parser::Match(idText,"maccmd"))
				Receive::MacCommandMessage(valueText);

			else if (Parser::Match(idText,"join"))
				Receive::JoinMessage(valueText);

			else if (Parser::Match(idText,"gw"))
				Receive::GatewayStatusMessage(valueText);

			else if (Parser::Match(idText,"command"))
				Receive::CommandMessage(valueText, source);

			else if (Parser::Match(idText,"ip"))
				Receive::IpControlMessage(valueText, source);

			else if (Parser::Match(idText,"ackreq"))
			{
				std::string acknowledgement = JSON::ReceiveAcknowledgementRequest(valueText);

				if (!acknowledgement.empty())
					JSON::Send(source, acknowledgement);	
			}
		}
	}
	catch (JSON::MessageRejectException const& e)
	{
		if (Debug::Print(Debug::monitor))
		{
			std::stringstream text;

			text << "Message rejected - " << e.Explanation();
					
			if (e.NumberSet())
				text << " 0x" << std::hex << e.Number();

			text << e.Message() << std::endl;

			Debug::Write(text);
		}
	}
}



sint8 JSON::Receive::ReadBooleanValue(char const input[])	//returns 0, 1 or -1 on error
{
	if (input[0] == 't' && input[1] == 'r' && input[2] == 'u' && input[3] == 'e')
		return 1;
	else if (input[0] == 'f' && input[1] == 'a' && input[2] == 'l' && input[3] == 's' && input[4] == 'e')
		return 0;

	return -1;
}


EuiType JSON::Receive::ReadEui(char const input[])
{
	EuiType result = invalidEui;
	uint8 euiArray[euiBytes];

	if (ConvertFixedLengthHexTextToBinaryArray(input, euiArray, euiBytes, true))
		result = ConvertBinaryArrayToUint64(euiArray, euiBytes);

	return result;
}


Position JSON::Receive::ReadPosition(char const receivedText[])
{
	char const* idText;
	JSON::Parser parser(receivedText);
	Position result;

	//lint -e{720} (Info -- Boolean test of assignment)
	while (idText = parser.FindName())
	{
		double scratchPad;
		char const* valueText = parser.FindValue();

		if (valueText == 0)
		{
			std::string message = "Unable to read ";
			message += idText;
			message += " value";
			throw JSON::MessageRejectException(receivedText, message);
		}

		else if (JSON::Parser::Match(idText, "lati"))
		{
			if (ReadRealNumber(valueText, scratchPad))
				result.latitude = scratchPad;
		}
		else if (JSON::Parser::Match(idText, "long"))
		{
			if (ReadRealNumber(valueText, scratchPad))
				result.longitude = scratchPad;
		}
		else if (JSON::Parser::Match(idText, "alti"))
		{
			if (ReadRealNumber(valueText, scratchPad))
				result.altitude = scratchPad;
		}
		else if (JSON::Parser::Match(idText, "tolv"))
		{
			if (ReadRealNumber(valueText, scratchPad))
				result.toleranceHorizonal = scratchPad;
		}
		else if (JSON::Parser::Match(idText, "tolh"))
		{
			if (ReadRealNumber(valueText, scratchPad))
				result.toleranceVertical = scratchPad;
		}
	}
	return result;
}


void JSON::Receive::AppMessage(char const receivedText[], MessageAddress const& source)
{
	char const* idText;
	JSON::Parser parser(receivedText);
	ValidValueEuiType moteEui;
	ValidValueUint16 token;
	LoRa::FrameApplicationDataSQ userData;
	MoteTransmitRecord transmitRecord;
	GatewayReceiveList gatewayReceiveList;
	ValidValueBool up;
	ValidValueUint32 sequenceNumber;

	//lint -e{720} (Info -- Boolean test of assignment)
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

		else if (JSON::Parser::Match(idText, "moteeui"))
			moteEui = ReadUnsignedLongInteger(valueText, true, true);

		else if (JSON::Parser::Match(idText, "dir"))
		{
			if (JSON::Parser::Match(valueText, "up"))
				up = true;
			else if (JSON::Parser::Match(valueText, "dn"))
				up = false;
		}

		else if (JSON::Parser::Match(idText,"seqno"))
		{
			uint32 readValue = ReadUnsignedInteger(valueText);

			if (readValue != ~uint32(0))
				sequenceNumber = readValue;
		}

		else if (JSON::Parser::Match(idText,"userdata"))
			ReadUserData(valueText, userData);

		else if (JSON::Parser::Match(idText,"motetx"))
			ReadMoteTransmitRecord(valueText, transmitRecord);

		else if (JSON::Parser::Match(idText,"gwrx"))
		{
			if (valueText[0] == '[')
			{
				//is array
				JSON::Parser arrayParser(valueText+1, false);

				while (valueText = arrayParser.FindValue(true))
				{
					ParseGatewayReceiveRecord(valueText, gatewayReceiveList);
				}
				parser.Update(arrayParser);
			}
			else
			{
				ParseGatewayReceiveRecord(valueText, gatewayReceiveList);
			}
		}

		else if (JSON::Parser::Match(idText,"token"))
		{
			uint32 readValue = ReadUnsignedInteger(valueText);

			if (readValue > 0xFFFF)
				throw JSON::MessageRejectException(receivedText, "Unable to read token value");

			token = readValue;
		}
	}

	if (!moteEui.Valid())
		throw JSON::MessageRejectException(receivedText, "Unable to parse app message");

	if (!up && (userData.Length() > LoRa::maxDownstreamDataBytes))
		throw JSON::MessageRejectException(receivedText, "Downstream message is too long");

	Receive::AppDataMessage(moteEui, sequenceNumber, token, userData, transmitRecord, gatewayReceiveList, source, up);
}


void JSON::Receive::MacCommandMessage(char const receivedText[])
{
	char const* idText;
	JSON::Parser parser(receivedText);
	ValidValueEuiType moteEui;
	ValidValueUint16 token;
	LoRa::OptionRecord macCommand;

	//lint -e{720} (Info -- Boolean test of assignment)
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

		else if (JSON::Parser::Match(idText, "moteeui"))
			moteEui = ReadUnsignedLongInteger(valueText, true, true);

		else if (JSON::Parser::Match(idText,"token"))
		{
			uint32 readValue = ReadUnsignedInteger(valueText);

			if (readValue > 0xFFFF)
				throw JSON::MessageRejectException(receivedText, "Unable to read token value");

			token = readValue;
		}

		else if (JSON::Parser::Match(idText, "command"))
		{
			sint16 length = ConvertBase64TextToBinaryArray(valueText, macCommand.DataNonConst(), macCommand.MaxLength());

			if (length >= 0)
			{
				macCommand.SetLength(length);

				if (Debug::Print(Debug::verbose))
				{
					std::stringstream text;
					text << ConvertBinaryArrayToHexTextBlock("Received MAC command", macCommand.Data(), macCommand.Length());
					Debug::Write(text);
				}

				if (macCommand.Length() == 0)
					throw MessageRejectException(receivedText, std::string("Data invalid :") +  ReadValue(valueText));
			}
		}
	}

	if (!moteEui.Valid())
		throw JSON::MessageRejectException(receivedText, "Unable to parse app message");

	Receive::MacCommandMessage(moteEui, token, macCommand);
}


void JSON::Receive::GatewayStatusMessage(char const receivedText[])
{
	char const*			idText;
	JSON::Parser		parser(receivedText);
	ValidValueEuiType	gatewayEui;
	Position			position;
	ValidValueBool		gps;
	LoRa::ValidRegion	region;

	//lint -e{720} (Info -- Boolean test of assignment)
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
			gatewayEui = ReadUnsignedLongInteger(valueText, true, true);

		else if (JSON::Parser::Match(idText, "posn"))
			position = ReadPosition(valueText);

		else if (JSON::Parser::Match(idText, "gps"))
		{
			sint8 readValue = ReadBoolean(valueText);

			if (readValue < 0)
				continue;

			gps = readValue ? true : false;
		}

		else if (JSON::Parser::Match(idText, "loraregion"))
		{
			std::string regionName = ReadValue(valueText);
			LoRa::Region readValue = LoRa::ReadRegion(regionName);

			if (readValue < LoRa::numberOfRegions)
				region = readValue;
		}
	}

	if (!gatewayEui.Valid())
		return;

	GatewayStatus(gatewayEui, gps, position, region);
}


void JSON::Receive::ReadUserData(char const receivedText[], LoRa::FrameApplicationDataSQ& output)
{
	char const* idText;
	JSON::Parser parser(receivedText);

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

		else if (JSON::Parser::Match(idText,"port"))
			output.Port(ReadUnsignedInteger(valueText));

		else if (JSON::Parser::Match(idText,"payload"))
		{
			sint16 readBytes = ConvertBase64TextToBinaryArray(valueText, output.DataNonConst(), output.MaxLength());

			if (readBytes < 0)
				throw JSON::MessageRejectException(receivedText, "Unable to read appdata.msg");

			output.SetLength(readBytes);
		}
	}
}


void JSON::Receive::ReadMoteTransmitRecord(char const receivedText[], MoteTransmitRecord& output)
{
	char const* idText;
	JSON::Parser parser(receivedText);

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

		else if (Parser::Match(idText,"freq"))
		{
			uint32 frequency_Hz = ReadUnsignedFixPointNumberAsInteger(valueText,6);

			if (frequency_Hz > 0 && frequency_Hz != ~uint32(0))
				output.frequency_Hz = frequency_Hz;
			else
				throw MessageRejectException(receivedText, "Invalid freq value");
		}
		else if (Parser::Match(idText,"codr"))
		{
			LoRa::CodeRate codeRate = valueText;

			if (!codeRate.Valid())
				throw MessageRejectException(receivedText, "Unable to read coding rate value");

			output.codeRate = codeRate;
		}
		else if (Parser::Match(idText,"datr"))
		{
			LoRa::DataRate dataRate = valueText;
			if (!dataRate.Valid())
				throw MessageRejectException(receivedText, "Unable to read datarate ");

			output.dataRate = dataRate;
		}
		else if (Parser::Match(idText,"adr"))
		{
			sint8 read = ReadBooleanValue(valueText);

			if (read == -1)
				throw MessageRejectException(receivedText, "Unable to read adr value");

			output.adrEnabled = bool(read == 1);
		}
	}
}


void JSON::Receive::ReadGatewayReceiveRecord(char const receivedText[], GatewayReceiveRecord& output)
{
	char const* idText;
	JSON::Parser parser(receivedText);

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

		else if (JSON::Parser::Match(idText, "eui"))
			output.eui = ReadUnsignedLongInteger(valueText, true, true);

		else if (JSON::Parser::Match(idText,"time"))
		{
			output.receiveTime = valueText;
			output.receiveTime.Accurate(true);
		}

		else if (JSON::Parser::Match(idText,"timefromgateway"))
			output.receiveTime.Accurate(JSON::Parser::Match(valueText,"true"));

		else if (JSON::Parser::Match(idText,"chan"))
		{
			uint32 channel = ReadUnsignedInteger(valueText);

			if (channel < ~uint32(0))
				output.channel = uint8(channel);
		}
		else if (Parser::Match(idText,"rfch"))
		{
			uint8 rfChain = uint8(ReadUnsignedInteger(valueText));

			output.rfChain = rfChain;
		}
		else if (Parser::Match(idText,"rssi"))
		{
			signed signalStrength_dBm = ReadSignedInteger(valueText);

			if (signalStrength_dBm > SHRT_MIN)
				output.signalStrength_cBm = signalStrength_dBm * 10;
			else
				throw MessageRejectException(receivedText, "Invalid rssi value");
		}
		else if (Parser::Match(idText,"lsnr"))
		{
			sint16 signalToNoiseRatio_cB = sint16(ReadSignedFixPointNumberAsInteger(valueText, 1));

			if (signalToNoiseRatio_cB > SHRT_MIN)
				output.signalToNoiseRatio_cB = signalToNoiseRatio_cB;
			else
				throw MessageRejectException(receivedText, "Invalid lsnr value");
		}
	}
}


void JSON::Receive::ParseGatewayReceiveRecord(char const valueText[], GatewayReceiveList& list)
{
	GatewayReceiveRecord* record = new GatewayReceiveRecord;

	ReadGatewayReceiveRecord(valueText, *record);

	list.Add(record);
}

