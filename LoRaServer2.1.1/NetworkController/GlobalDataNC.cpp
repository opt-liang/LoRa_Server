/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "GlobalDataNC.hpp"


namespace Global
{
	const char							programDescription[] = "Network controller";

	std::string							databaseHost = "localhost";
	std::string							databaseName = "lora_networkcontroller";

	LoRa::DatabaseNC					networkControllerDatabase;
}

SqlDb::Server&							ConfiguredValueBaseType::database = Global::networkControllerDatabase;	//declared in ConfiguredValue.hpp


namespace Global
{
	MoteList							moteList(false);
	GatewayList							gatewayList(false, timeThreadTickPeriod_ms);
	EuiReminderQueue::Queue				moteReminderQueue;

	ConfiguredUint16					defaultMoteChannelMask("defaultMoteChannelMask", LoRa::defaultMoteChannelMask, true);	//represent as hex
	ConfiguredUint16					defaultMoteChannelMaskControl("defaultMoteChannelMaskControl", LoRa::defaultMoteChannelMaskControl);
	ConfiguredUint16					transmissionsOfUnacknowledgedUplinkFrame("maxTxsOfUplinkFrame", LoRa::defaultMoteTransmissionsOfUnacknowledgedUplinkFrame);

	LoRa::ApplicationDatabase&			genericDatabase = Global::networkControllerDatabase;
}

