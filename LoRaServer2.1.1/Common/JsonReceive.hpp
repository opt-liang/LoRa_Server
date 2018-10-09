/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef JSON_RECEIVE_HPP
#define JSON_RECEIVE_HPP

#include "General.h"
#include "Eui.hpp"
#include "LoRa.hpp"
#include "MessageAddress.hpp"
#include "JsonException.hpp"
#include "TransmissionRecord.hpp"

#include <string>

struct Position;

namespace JSON
{
	namespace Receive
	{
		sint8 ReadBooleanValue(char const input[]);	//returns 0, 1 or -1 on error
		EuiType ReadEui(char const input[]);
		Position ReadPosition(char const text[]);

		void Top(char const receivedText[], MessageAddress const& source, bool allowDataTraffic);
			// if allowDataTraffic is false, accept only configuration commands
		void AppMessage(char const receivedText[], MessageAddress const& source);
		void MoteMessage(char const receivedText[], MessageAddress const& source);
		void MacCommandMessage(char const receivedText[]);

		void GatewayStatusMessage(char const receivedText[]);	//received from another server, not the gateway

		void CommandMessage(char const receivedText[], MessageAddress const& source);
		void IpControlMessage(char const receivedText[], MessageAddress const& source);
		void JoinMessage(char const receivedText[]);

		void ReadUserData(char const receivedText[], LoRa::FrameApplicationDataSQ& output);
		void ReadMoteTransmitRecord(char const receivedText[], MoteTransmitRecord& output);
		void ReadGatewayReceiveRecord(char const receivedText[], GatewayReceiveRecord& output);

		void ParseGatewayReceiveRecord(char const valueText[], GatewayReceiveList& list);

		//Must be defined by client
		void AppDataMessage(EuiType moteEui, ValidValueUint32 const& sequenceNumber, ValidValueUint16 const& token, 
			LoRa::FrameApplicationData const& payload, MoteTransmitRecord const& transmitRecord, GatewayReceiveList const& gatewayReceiveList, 
			MessageAddress const& source, bool up);

		void MacCommandMessage(EuiType moteEui, ValidValueUint16 const& token, LoRa::OptionRecord command);

		void GatewayStatus(EuiType eui, ValidValueBool const& gps, Position const& position, LoRa::ValidRegion const& region);	//received from another server, not the gateway
	}
}

#endif

