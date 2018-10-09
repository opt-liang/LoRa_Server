/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef CUSTOMER_GLOBAL_DATA_HPP
#define CUSTOMER_GLOBAL_DATA_HPP

#include "GlobalData.hpp"
#include "LoRaDatabaseCS.hpp"
#include "ConfiguredValue.hpp"
#include "MoteCS.hpp"
#include "ApplicationDataOutput.hpp"

namespace Global
{
	// Data that is common within a server

	extern LoRa::DatabaseCS						customerDatabase;	//must be declared AFTER databaseHost, databaseName, databaseUser, databasePassword

	extern ApplicationDataOutput				applicationDataOutput;

	extern MoteList								moteList;
}

#endif

