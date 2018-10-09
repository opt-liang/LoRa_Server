/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "GlobalDataCS.hpp"

namespace Global
{
	const char							programDescription[] = "Customer server";

	std::string							databaseHost = "localhost";
	std::string							databaseName = "lora_customer";

	LoRa::DatabaseCS					customerDatabase;
}

//these objects rely on customerDatabase being initialised
SqlDb::Server&							ConfiguredValueBaseType::database = Global::customerDatabase;	//declared in ConfiguredValue.hpp


namespace Global
{
	ApplicationDataOutput				applicationDataOutput;

	MoteList							moteList(false);
	LoRa::ApplicationDatabase&			genericDatabase = Global::customerDatabase;
}

