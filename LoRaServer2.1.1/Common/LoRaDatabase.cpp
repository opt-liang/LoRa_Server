/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "LoRaDatabase.hpp"
#include "BuildVersion.hpp"


bool LoRa::Database::SetConfiguredValue(const char name[], const char value[])
{
	std::stringstream query;

	query << "REPLACE INTO configuration SET name = \"" << name << "\", value = \"" << value << "\"";

	return db.Query(query);
}

bool LoRa::Database::GetConfiguredValue(const char name[], std::string& value)
{
	std::stringstream query;
	query << "SELECT value FROM configuration WHERE name = \"" << name << '\"';

	SqlDb::Client client(db);
	try
	{
		client.Query(query, true);
	}
	catch (SqlDb::Exception const&)
	{
		return false;
	}

	if (!client.GetNextRow())
		return false;

	value = client.GetFieldValue(0);
	return true;
}

std::string LoRa::Database::GetConfiguredValue(const char name[])
{
	std::string result;

	GetConfiguredValue(name, result);
	return result;
}


void LoRa::Database::UpdateBuildTime()
{
	SetConfiguredValue("buildDate", BuildVersion::Date());
	SetConfiguredValue("buildTime", BuildVersion::Time());
	SetConfiguredValue("buildVersion", BuildVersion::VersionString());
}


bool LoRa::Database::UpdateStructure()
{
	bool result = true;

	result &= SetServersAppEuiToUnsigned();
	result &= UpdateDBSpecificStructure();

	return result;
}

