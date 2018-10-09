/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "GatewayMessageProtocol.hpp"
#include "UdpSocket.hpp"
#include "ExceptionClass.hpp"


void GatewayMessageProtocol::Parser::TransmitAcknowledgement(UDP::Socket const& txSocket, MessageType msgType) const
{
	uint8 data[headerLength];

	data[versionOffset] = 1; //Version
	memcpy(&data[tokenOffset], &receivedData[tokenOffset], sizeof(uint16));	//token
	data[messageTypeOffset] = uint8(msgType);

	txSocket.Send(source,data,headerLength);
}


void GatewayMessageProtocol::TransmitMessage(UDP::Socket const& txSocket, MessageType msgType, sockaddr_in const& destination, EuiType const& gatewayEui, uint16 token, uint8 const payload[], uint16 payloadLength)
{
	uint8 txUdpPayload[maxTransmittedUdpMessage];

	txUdpPayload[0] = 1; //version
	txUdpPayload[1] = uint8(token >> 8);		//pullResponse token should always be zero
	txUdpPayload[2] = uint8(token);
	txUdpPayload[3] = uint8(msgType);

	uint8 *ptr = &txUdpPayload[headerLength];

	if (msgType == pushData || msgType == pullData)
		ptr = Write8ByteValue(ptr, gatewayEui);

	if (payloadLength > 0)
	{
		memcpy(ptr, payload, payloadLength);
		ptr += payloadLength;
	}

	txSocket.Send(destination, txUdpPayload, uint16(ptr - txUdpPayload));
}

uint32 GatewayMessageProtocol::Read4ByteValue(uint8 const input[])
{
	uint32 result;
	
	result = uint32(input[0]);
	result <<= 8, result |= uint32(input[1]);
	result <<= 8, result |= uint32(input[2]);
	result <<= 8, result |= uint32(input[3]);

	return result;
}


uint64 GatewayMessageProtocol::Read8ByteValue(uint8 const input[])
{
	uint64 result = Read4ByteValue(input);
	result <<= (4 * 8), result |= Read4ByteValue(input + 4);

	return result;
}

uint8* GatewayMessageProtocol::Write4ByteValue(uint8 output[], uint32 input)
{
	uint8* end = output + 4;
	uint8* ptr = end;

	*(--ptr) = uint8(input), input >>= 8;
	*(--ptr) = uint8(input), input >>= 8;
	*(--ptr) = uint8(input), input >>= 8;
	*(--ptr) = uint8(input);

	return end;
}

uint8* GatewayMessageProtocol::Write8ByteValue(uint8 output[], uint64 input)
{
	uint8* ptr = output;
	
	ptr = Write4ByteValue(ptr, uint32(input >> 32));
	return Write4ByteValue(ptr, uint32(input));
}

