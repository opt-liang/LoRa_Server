/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef APPLICATION_GLOBAL_DATA_HPP
#define APPLICATION_GLOBAL_DATA_HPP

#include "GlobalData.hpp"
#include "LoRaDatabaseAS.hpp"
#include "ConfiguredValue.hpp"
#include "ConfiguredValueLoRa.hpp"
#include "JoinControllerAS.hpp"
#include "MoteAS.hpp"

namespace Global
{
	// Data that is common within a server

	extern LoRa::DatabaseAS						applicationDatabase;

	extern JoinController						joinController;
	extern MoteList								moteList;
	extern ConfiguredBool						allowDuplicateMoteNonce;
	extern LoRa::ConfiguredCypherKey			encryptionKeyDefault;
}

#endif

