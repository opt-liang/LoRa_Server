/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "LoRaApplicationDatabase.hpp"


bool LoRa::ApplicationDatabase::CreateApplication(EuiType eui, std::string const& name, std::string const& owner)
{
	std::stringstream query;

	query << "INSERT INTO applications (eui, name, owner) VALUES (" << eui << ", \"" << name << "\", \"" << owner << "\")";

	return db.Query(query);
}


bool LoRa::ApplicationDatabase::CreateApplicationServer(EuiType eui, sockaddr_in const& address, bool active, Service::Mask serviceMask)
{
	bool const nullApplication = eui == invalidEui;
	std::stringstream query;

	query << "INSERT INTO servers (";
		
	if (!nullApplication)
		query << "appEui, ";
	
	query << "addressText, nullApplication, active, serviceText) VALUES (";

	if (!nullApplication)
		query << eui << ", ";
	
	query << '\"' << ConvertPortAddressToSqlSafeString(address) << "\", " << (nullApplication ? 1 : 0) << ", " <<
		(active ? 1 : 0) << ", \"" << Service::TextString(serviceMask) << "\")";

	return db.Query(query);
}


bool LoRa::ApplicationDatabase::UpdateApplicationServer(::EuiType eui, sockaddr_in const& address, Service::Mask serviceMask)
{
	bool const nullApplication = eui == invalidEui;
	std::stringstream query;

	query << "UPDATE servers SET serviceText = \"" << Service::TextString(serviceMask) << "\" WHERE appEui ";
		
	if (!nullApplication)
		query << "= " << eui;
	else
		query << "IS NULL";

	query << " AND addressText = \"" << ConvertPortAddressToSqlSafeString(address) << "\"";

	return db.Query(query);
}


bool LoRa::ApplicationDatabase::DeleteApplication(EuiType eui)
{
	std::stringstream query;

	query << "DELETE FROM applications WHERE HEX(eui) = \'" << std::hex << eui << '\'';

	return db.Query(query);
}

bool LoRa::ApplicationDatabase::DeleteApplicationServers(::EuiType eui)
{
	bool const nullApplication = eui == invalidEui;
	std::stringstream query;

	query << "DELETE FROM servers WHERE ";
	if (!nullApplication)
		query << "HEX(appEui) = \'" << std::hex << eui << "\' AND nullApplication = 0";
	else
		query << "nullApplication = 1";

	return db.Query(query);
}


bool LoRa::ApplicationDatabase::DeleteApplicationServer(EuiType eui, sockaddr_in const& serverAddress)
{
	bool const nullApplication = eui == invalidEui;
	std::stringstream query;

	query << "DELETE FROM servers WHERE ";
	if (!nullApplication)
		query << "HEX(appEui) = \'" << std::hex << eui << "\' AND nullApplication = 0";
	else
		query << "nullApplication = 1";

	query << " AND addressText = \"" << ConvertPortAddressToSqlSafeString(serverAddress) << "\"";
	return db.Query(query);
}


LoRa::ApplicationDatabase::ApplicationRecord LoRa::ApplicationDatabase::ApplicationClient::Read()
{
	LoRa::ApplicationDatabase::ApplicationRecord application;

	if (!GetNextRow())
	{
		application.SetInvalid();
		return application;
	}

	application.eui  = ReadUnsignedLongInteger(GetFieldValue(0));
	application.name  = GetFieldValue(1);
	application.owner = GetFieldValue(2);

	return application;
}


LoRa::ApplicationDatabase::ServerRecord LoRa::ApplicationDatabase::ServerClient::Read()
{
	LoRa::ApplicationDatabase::ServerRecord serverRecord;

	if (!GetNextRow())
	{
		serverRecord.SetInvalid();
		return serverRecord;
	}

	serverRecord.address = ApplicationDatabase::ReadPortAddressFromSqlSafeString(GetFieldValue(0));
	serverRecord.active = ::ReadUnsignedInteger(GetFieldValue(1)) ? true : false;
	serverRecord.mask = Service::ReadMask(GetFieldValue(2));

	return serverRecord;
}


std::string LoRa::ApplicationDatabase::ConvertPortAddressToSqlSafeString(sockaddr_in const& address)	//required for fields that are searched on because '.' characters are treated specially in SQL
{
	std::string text = AddressText(address);

	for (std::string::iterator it = text.begin();it != text.end(); it++)
	{
		if (*it == '.')
			*it = sqlDotSubstitutionCharacter;
	}

	return text;
}


sockaddr_in LoRa::ApplicationDatabase::ReadPortAddressFromSqlSafeString(char const text[])
{
	std::string localText = text;

	for (std::string::iterator it = localText.begin();it != localText.end(); it++)
	{
		if (*it == sqlDotSubstitutionCharacter)
			*it = '.';
	}

	return ReadIpAddress(localText);
}

