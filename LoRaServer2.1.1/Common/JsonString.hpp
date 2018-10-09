/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef JSON_GENERATOR_HPP
#define JSON_GENERATOR_HPP

#include "LoRa.hpp"
#include "General.h"
#include <string.h>
#include <sstream>
#include <iomanip>

class MoteTransmitRecord;
class GatewayReceiveRecord;
class GatewayReceiveList;
struct Position;

namespace JSON
{
	class String : public std::string
	{
		//lint --e{1509} (Warning -- base class destructor for class 'basic_string' is not virtual)
	public:
		String() {}

		void AddQuotedText(char const text[])
		{
			*this += '\"';
			*this += text;
			*this += '\"';
		}

		void AddTextValue(char const name[], char const value[], bool addCommaBefore = true)
		{
			AddObjectName(name, addCommaBefore);
			AddQuotedText(value);
		}

		void AddUnquotedTextValue(char const name[], char const value[], bool addCommaBefore = true)
		{
			AddObjectName(name, addCommaBefore);
			*this += value;
		}

		void AddTextValue(char const name[], std::string const& value, bool addCommaBefore = true) {AddTextValue(name, value.c_str(), addCommaBefore);}
		void AddTextValue(char const name[], std::stringstream const& value, bool addCommaBefore = true) {AddTextValue(name, value.str(), addCommaBefore);}
		void AddUnquotedTextValue(char const name[], std::string const& value, bool addCommaBefore = true) {AddUnquotedTextValue(name, value.c_str(), addCommaBefore);}
		void AddUnquotedTextValue(char const name[], std::stringstream const& value, bool addCommaBefore = true) {AddUnquotedTextValue(name, value.str(), addCommaBefore);}

		void AddSignedValue(char const name[], sint64 input, bool addCommaBefore = true)
		{
			AddObjectName(name, addCommaBefore);

			std::stringstream valueText;
			valueText << input;

			*this += valueText.str();
		}

		void AddUnsignedValue(char const name[], uint64 input, bool addCommaBefore = true)	{AddUnsignedValue(name, input, false,  addCommaBefore);}
		void AddUnsignedHexValue(char const name[], uint64 input, bool addCommaBefore = true) {AddUnsignedValue(name, input, true, addCommaBefore);}

		void AddUnsignedHexValue(char const name[], uint8 const input[], uint16 length, bool addCommaBefore = true)	//assumes input and output endians are  the same
		{
			AddObjectName(name, addCommaBefore);

			*this += '\"';
			*this += ConvertBinaryArrayToHexText(input, length, false);
			*this += '\"';
		}

		void AddAcknowledgementRequest(uint16 id, bool addCommaBefore = true) {AddUnsignedValue("ackreq", id, addCommaBefore);}
		void AddAcknowledgement(uint16 id, bool addCommaBefore = true) {AddUnsignedValue("ack", id, addCommaBefore);}

		void AddFixedPointNumber(char const name[], bool includeTrailingZeros, sint32 value, uint32 unitValue, bool addCommaBefore = true)
		{
			std::string valueText = WriteSignedIntegerAsFixedPointNumber(value, unitValue, includeTrailingZeros);
			AddUnquotedTextValue(name, valueText, addCommaBefore);
		}

		void AddBooleanValue(char const name[], bool value, bool addCommaBefore = true)
		{
			AddObjectName(name, addCommaBefore);
			*this += (value ? "true" : "false");
		}

		void AddFloatValue(char const name[], float value, uint decimalPlaces, bool addCommaBefore = true) 
			{AddDoubleValue(name, value, decimalPlaces, addCommaBefore);}

		void AddDoubleValue(char const name[], double value, uint decimalPlaces, bool addCommaBefore = true)
		{
			AddObjectName(name, addCommaBefore);

			uint16 integerDigits = FindDigitsToLeftOfDecimalPoint(value);

			std::stringstream text;
			text << std::setprecision(integerDigits + decimalPlaces) << value;

			*this += text.str();
		}

		void AddDataAsBase64(char const name[], uint8 const data[], uint16 bytes, bool addCommaBefore = true)
		{
			AddObjectName(name, addCommaBefore);

			char base64Text[LoRa::maxBase64TextLength];
			ConvertBinaryArrayToBase64Text(data, bytes, base64Text, false);

			*this += '\"';
			*this += base64Text;
			*this += '\"';
		}

		void AddObjectName(char const name[], bool addCommaBefore = true)
		{
			if (addCommaBefore)
				Comma();

			AddQuotedText(name);
			Colon();
		}

		void OpenStructuredObject(char const name[], bool addCommaBefore = true)	//must be closed by a Close()
		{
			AddObjectName(name, addCommaBefore);
			Open();
		}

		void OpenArray(char const name[], bool addCommaBefore = true)
		{
			AddObjectName(name, addCommaBefore);
			OpenArray();
		}

		void Comma() {*this += ',';}
		void Open()  {*this += '{';}
		void Close() {*this += '}';}
		void Colon() {*this += ':';}
		void OpenArray()  {*this += '[';}
		void CloseArray() {*this += ']';}

		void AddEui(char const name[], EuiType eui, bool addCommaBefore = true) {AddUnsignedHexValue(name, eui, addCommaBefore);}
		void AddDirection(bool up, bool addCommaBefore = true) {AddTextValue("dir", up ? "up" : "dn", addCommaBefore);}
		void AddDataRate(LoRa::DataRate const& dataRate, bool addCommaBefore = true);

		//Gateway methods
		void AddPositionObject(bool positionIsFromGPS, Position const& position, bool addCommaBefore = true);
		void AddPosition(Position const& position, bool addCommaBefore = true);

		//FRAME methods
		void AddAppObject(bool up, EuiType eui, ValidValueUint32 const& sequenceNumber, ValidValueUint16 const& token, 
			LoRa::FrameApplicationData const& payload,
			MoteTransmitRecord const& transmitRecord,
			GatewayReceiveList const& gatewayList,
			bool addCommaBefore = true);

		void AddUserData(LoRa::FrameApplicationData const& payload, bool addCommaBefore = true);
		void AddMacCommand(EuiType mote, ValidValueUint16 const& token, LoRa::OptionRecord const& command, bool addCommaBefore = true);
		void AddMacCommand(EuiType mote, ValidValueUint16 const& token, LoRa::FrameDataRecord const& command, bool addCommaBefore = true);
		void AddMoteTxMetaData(MoteTransmitRecord const& transmitRecord, bool addCommaBefore = true);
		void AddGatewayRxMetaData(GatewayReceiveList const& list, bool addCommaBefore = true);
		void AddGatewayRxMetaData(bool includeName, GatewayReceiveRecord const& record, bool addCommaBefore = true);

		//MOTE methods
		void AddMessageSent(EuiType mote, bool app, uint16 token, bool addCommaBefore = true);
		void AddResetDetected(EuiType mote, bool addCommaBefore = true);
		void AddAcknowledgementReceived(EuiType mote, bool app, bool addCommaBefore = true);
		void AddQueueLengthQuery(EuiType mote, bool app, bool addCommaBefore = true);
		void AddQueueLength(EuiType mote, bool app, uint16 length, bool addCommaBefore = true);
		void AddSequenceNumberRequest(EuiType mote, bool addCommaBefore = true);
		void AddSequenceNumberGrant(EuiType mote, uint32 sequenceNumber, bool addCommaBefore = true);

		//Join methods
		void AddJoinRequestNtoA(LoRa::Frame const& frame, bool addCommaBefore = true);
		void AddJoinDetailsNtoA(EuiType moteEui, EuiType appEui, uint32 moteNetworkAddress, uint16 deviceNonce, bool addCommaBefore = true);
		void AddJoinNotificationAtoN(EuiType mote, EuiType app, bool addCommaBefore = true);
		void AddJoinAccept(EuiType moteEui, bool accept);
		void AddJoinComplete(EuiType moteEui, LoRa::CypherKey const& networkSessionKey, LoRa::Frame const& frame);

	private:
		void AddUnsignedValue(char const name[], uint64 input, bool hex, bool addCommaBefore)
		{
			AddObjectName(name, addCommaBefore);

			std::stringstream valueText;

			if (hex)
				valueText << std::hex << '\"';

			valueText << input;

			if (hex)
				valueText << '\"';

			*this += valueText.str();
		}

		static uint8 FindDigitsToLeftOfDecimalPoint(double input)
		/* an input of 0 to 0.999 returns 0, 1 to 9.9999 returns 1, 10 - 99.999 return 2 etc.  
		This function can only be used when 'input' is less than 4E9 */
		{
			//find how many digits there are left of the decimal point
			uint64 integerValue = static_cast<uint64>(fabs(input));

			/* Avoid rounding errrors (only important where the number of significant bits in the integer value 
			exceeds the number of mantissa bits) */
			if (integerValue < fabs(input))
				integerValue++;
			uint8 result = 0;

			while(integerValue > 0)
			{
				integerValue /= 10;
				result++;
			}
			return result;
		}
	};
}

#endif

