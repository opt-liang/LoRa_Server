/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "GatewayMessageProtocol.hpp"

void GatewayMessageProtocol::Parser::Parse()
{
	if (PacketLength() < headerLength)
		throw SyntaxError(this, "Packet too short");

	if (Version() != 1)
		throw SyntaxError(this, "Incorrect version");

	MessageType msgType = Type();

	MessageType acknowledgeType = numberOfMessageTypes;
	bool carriesData = false;
	switch (msgType)
	{
	case pushData:		acknowledgeType = pushAck;	carriesData = true;		break;
	case pushAck:															break;
	case pullData:		acknowledgeType = pullAck;	carriesData = false;	break;
	case pullAck:															break;
	case pullResponse:								carriesData = true;		break;

	default:
		{
			std::stringstream text;

			text << "MesssageProtocol Unexpected message type 0x" << std::hex << int(msgType);
			throw SyntaxError(this, text);
		}
		break;
	}

	if (acknowledgeType < numberOfMessageTypes)
		TransmitAcknowledgement(socket, acknowledgeType);

	if (carriesData && DataPayloadLength() < 0)
		throw SyntaxError(this, "Negative length payload");

	switch (msgType)
	{
	case pushData:
		ReactToPushDataPacket();
		break;

	case pullData:
		ReactToPullDataPacket();
		break;

	case pullResponse:
		ReactToPullResponsePacket();
		break;

	case pushAck:
		ReactToPushAckPacket();
		break;

	case pullAck:
		ReactToPullAckPacket();
		break;
	}
}
