/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "GlobalDataNS.hpp"
#include "LoRaRegion.hpp"

namespace Global
{
	const char							programDescription[] = "Network server";

	std::string							databaseHost = "localhost";
	std::string							databaseName = "lora_network";

	LoRa::DatabaseNS					networkDatabase;
}

SqlDb::Server&							ConfiguredValueBaseType::database = Global::networkDatabase;	//declared in ConfiguredValue.hpp


namespace Global
{
	UDP::Socket							messageProtocolSocket;
	UDP::Socket							jsonSocket;
	IP::SocketSet						udpSocketSet(true);	//use round robin servicing

	MoteList							moteList(false);
	GatewayList							gatewayList(false);
	JoinController						joinController(timeThreadTickPeriod_ms);

	static const uint8					authenticationKeyFixedDefaultValueArray[] = {0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C};
	static const LoRa::CypherKey		authenticationKeyFixedDefaultValue = authenticationKeyFixedDefaultValueArray;

	ConfiguredDataRate					dataRateWindow1Americas902("dataRate2ndWindowAmericas902", LoRa::Americas902::defaultWindow1DataRate);
	ConfiguredDataRate					dataRateWindow1China779("dataRate2ndWindowChina779", LoRa::China779::defaultWindow1DataRate);
	ConfiguredDataRate					dataRateWindow1Eu433("dataRate2ndWindowEur433", LoRa::Europe433::defaultWindow1DataRate);
	ConfiguredDataRate					dataRateWindow1Eu863("dataRate2ndWindowEur863", LoRa::Europe863::defaultWindow1DataRate);
	LoRa::ConfiguredCypherKey			defaultAuthenticationKey("defaultAuthenticationKey", authenticationKeyFixedDefaultValue);
	ConfiguredUint16					defaultMoteChannelMask("defaultMoteChannelMask", LoRa::defaultMoteChannelMask, true);	//represent as hex
	ConfiguredUint16					defaultMoteChannelMaskControl("defaultMoteChannelMaskControl", LoRa::defaultMoteChannelMaskControl);
	ConfiguredRegion					defaultGatewayRegion("defaultGatewayRegion", LoRa::europe863);
	ConfiguredUint16					gatewayToNetworkServerMaxDelay_ms("netdelayup_ms", Gateway::defaultDelayToNetworkServer_ms);
	ConfiguredSint16					gatewayTxPower_dBm("gatewayTxPower_dBm", Gateway::defaultTransmitPower_dBm);
	ConfiguredUint16					networkServerToGatewayMaxDelay_ms("netdelaydown_ms", Gateway::defaultDelayFromNetworkServer_ms);
	ConfiguredUint16					moteReceiveWindow("moteReceiveWindow", Mote::defaultReceiveWindow);
	ConfiguredUint32					moteMissSeqNoSearchLimit("moteMissSeqNoSearchLimit", Mote::defaultMissingSeqNoSearchLimit);
										//the number of interpolated value of MS 16 bit sequence numbers to be tested when valid value cannot be found due to missing frame
	ConfiguredUint32					moteMissSeqNoSearchRetries("moteMissSeqNoSearchRetries", Mote::defaultMissingSeqNoSearchRetries);
										//the number of times a range of the most significant part of sequence numbers is tested before it is assumed to not contain the correct one
	ConfiguredUint32					moteResetSeqNoSearchLimit("moteResetSeqNoSearchLimit", Mote::defaultResetSeqNoSearchLimit);
										//the number of interpolated value of MS 16 bit sequence numbers to be tested when valid value cannot be found due to mote reset
	ConfiguredUint32					timeToAssumeMoteLost_s("timeToAssumeMoteLost_s", Mote::defaultTimeToAssumeContactLost_s);
	ConfiguredUint32					timeToAssumeMoteReset_s("timeToAssumeMoteReset_s", Mote::defaultTimeToAssumeReset_s);
	ConfiguredUint16					transmissionsOfUnacknowledgedUplinkFrame("maxTxsOfUplinkFrame", LoRa::defaultMoteTransmissionsOfUnacknowledgedUplinkFrame);


	Transmit::Queue						transmitQueue(moteList);
	FrameReceptionList					frameReceptionList;

	LoRa::ApplicationDatabase&			genericDatabase = Global::networkDatabase;
}

