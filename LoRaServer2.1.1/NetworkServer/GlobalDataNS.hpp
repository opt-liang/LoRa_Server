/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef NETWORK_GLOBAL_DATA_HPP
#define NETWORK_GLOBAL_DATA_HPP

#include "GlobalData.hpp"
#include "LoRa.hpp"
#include "MoteNS.hpp"
#include "LoRaDatabaseNS.hpp"
#include "ConfiguredValue.hpp"
#include "ConfiguredValueLoRa.hpp"
#include "TransmitQueue.hpp"
#include "CommandLineInterface.hpp"
#include "GatewayNS.hpp"
#include "JoinControllerNS.hpp"
#include "FrameReception.hpp"

typedef  ConfiguredValueNonIntegerTemplate<LoRa::Region> ConfiguredRegion;
typedef  ConfiguredValueNonIntegerTemplate<LoRa::DataRate> ConfiguredDataRate;

namespace Global
{
	// Data that is common within a server

	extern LoRa::DatabaseNS						networkDatabase;
	//must be declared AFTER databaseHost, databaseName, databaseUser, databasePassword
	//must be declared BEFORE moteList, gatewayList, clientApplicationList


	extern UDP::Socket							messageProtocolSocket;
	extern UDP::Socket							jsonSocket;
	extern IP::SocketSet						udpSocketSet;

	extern MoteList								moteList;
	extern GatewayList							gatewayList;
	extern JoinController						joinController;

	extern ConfiguredDataRate					dataRateWindow1Americas902;
	extern ConfiguredDataRate					dataRateWindow1China779;
	extern ConfiguredDataRate					dataRateWindow1Eu433;
	extern ConfiguredDataRate					dataRateWindow1Eu863;
	extern LoRa::ConfiguredCypherKey			defaultAuthenticationKey;
	extern ConfiguredUint16						defaultMoteChannelMask;
	extern ConfiguredUint16						defaultMoteChannelMaskControl;
	extern ConfiguredRegion						defaultGatewayRegion;
	extern ConfiguredUint16						gatewayToNetworkServerMaxDelay_ms;
	extern ConfiguredSint16						gatewayTxPower_dBm;
	extern ConfiguredUint16						networkServerToGatewayMaxDelay_ms;
	extern ConfiguredUint16						moteReceiveWindow;
	extern ConfiguredUint32						moteResetSeqNoSearchLimit;
	extern ConfiguredUint32						moteMissSeqNoSearchRetries;
	extern ConfiguredUint32						moteMissSeqNoSearchLimit;
	extern ConfiguredUint32						timeToAssumeMoteLost_s;
	extern ConfiguredUint32						timeToAssumeMoteReset_s;
	extern ConfiguredUint16						transmissionsOfUnacknowledgedUplinkFrame;

	extern Transmit::Queue						transmitQueue;
	extern FrameReceptionList					frameReceptionList;

	LoRa::Region GetDefaultRegion();	//on error, returns LoRa::numberOfRegions
	LoRa::DataRate const& Get2ndWindowDataRate(LoRa::Region region);

	bool CreateProvisionedMote(EuiType moteEui, EuiType const& appEui, uint32 networkAddress, LoRa::CypherKey const& authenticationKey);
	bool DeleteProvisionedMote(EuiType moteEui);
}

#endif

