/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "GatewayMessageProtocol.hpp"
#include "JsonReceiveNS.hpp"
#include "GlobalDataNS.hpp"

void GatewayMessageProtocol::Parser::ReactToPushDataPacket() const
{
	EuiType gatewayEui = Read8ByteValue(GatewayEui());

	Global::gatewayList.GatewaySeen(gatewayEui);

	JSON::Receive::Top((char const*) DataPayload(), thisServerReceiveTime_ms, source, gatewayEui);
}


void GatewayMessageProtocol::Parser::ReactToPullDataPacket() const
{
	EuiType gatewayEui = Read8ByteValue(GatewayEui());

	Global::gatewayList.GatewaySeen(gatewayEui, &source);
}

void GatewayMessageProtocol::Parser::ReactToPullResponsePacket() const
{
	//never called
}

void GatewayMessageProtocol::Parser::ReactToAcknowledgePacket() const	//only called by ReactToPushAckPacket() and ReactToPullAckPacket()
{
	//never called
}

