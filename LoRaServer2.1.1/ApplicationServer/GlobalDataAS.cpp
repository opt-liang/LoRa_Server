/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "GlobalDataAS.hpp"

namespace Global
{
	const char							programDescription[] = "Application server";

	std::string							databaseHost = "localhost";
	std::string							databaseName = "lora_application";

	LoRa::DatabaseAS					applicationDatabase;
}

SqlDb::Server&							ConfiguredValueBaseType::database = Global::applicationDatabase;	//declared in ConfiguredValue.hpp


namespace Global
{
	//lint -e{1502} (Warning -- defined object 'Global::joinController' of type 'JoinController' has no nonstatic data members)
	JoinController						joinController;
	MoteList							moteList(false);

	ConfiguredBool						allowDuplicateMoteNonce("allowDuplicateMoteNonce", false);

	static const uint8					encryptionKeyFixedDefaultValueArray[] = {0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C};
	static const LoRa::CypherKey		encryptionKeyFixedDefaultValue = encryptionKeyFixedDefaultValueArray;
	LoRa::ConfiguredCypherKey			encryptionKeyDefault("defaultEncryptionKey", encryptionKeyFixedDefaultValue);

	LoRa::ApplicationDatabase&			genericDatabase = applicationDatabase;
}

