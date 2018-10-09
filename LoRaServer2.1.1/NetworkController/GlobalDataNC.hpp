/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef NETWORK_CONTROLLER_GLOBAL_DATA_HPP
#define NETWORK_CONTROLLER_GLOBAL_DATA_HPP

#include "GlobalData.hpp"
#include "LoRaDatabaseNC.hpp"
#include "ConfiguredValue.hpp"
#include "MoteNC.hpp"
#include "GatewayNC.hpp"
#include "EuiReminderQueue.hpp"

namespace Global
{
	// Data that is common within a server

	extern LoRa::DatabaseNC						networkControllerDatabase;	//must be declared AFTER databaseHost, databaseName, databaseUser, databasePassword

	extern MoteList								moteList;
	extern GatewayList							gatewayList;
	extern EuiReminderQueue::Queue				moteReminderQueue;

	extern ConfiguredUint16						defaultMoteChannelMask;
	extern ConfiguredUint16						defaultMoteChannelMaskControl;
	extern ConfiguredUint16						transmissionsOfUnacknowledgedUplinkFrame;
}

#endif

