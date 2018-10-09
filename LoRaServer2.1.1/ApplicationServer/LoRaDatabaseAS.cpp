/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "LoRaDatabaseAS.hpp"

LoRa::DatabaseAS::ActiveMoteRecord LoRa::DatabaseAS::ActiveMoteClient::Read(void)
{
	ActiveMoteRecord record;

	if (!GetNextRow())
	{
		record.moteEui = invalidEui;
		return record;
	}

	unsigned long long scratchPad;
	if (sscanf(GetFieldValue(0),"%llu", &scratchPad) < 1)
		throw Exception(Debug::major, __LINE__, __FILE__);

	record.moteEui = scratchPad;

	if (sscanf(GetFieldValue(1),"%llu", &scratchPad) < 1)
		throw Exception(Debug::major, __LINE__, __FILE__);

	record.appEui = scratchPad;

	record.sessionKey = GetFieldValue(2);

	if (sscanf(GetFieldValue(3),"%llu", &scratchPad) < 1)
		throw Exception(Debug::major, __LINE__, __FILE__);

	record.networkAddress = uint32(scratchPad);

	char const* appKeyText = GetFieldValue(4);
	if (strlen(appKeyText) > 0)
		record.applicationKey = appKeyText;

	return record;
}


LoRa::DatabaseAS::JoinMoteRecord LoRa::DatabaseAS::JoinMoteClient::Read(void)
{
	JoinMoteRecord record;

	if (!GetNextRow())
	{
		record.moteEui = invalidEui;
		return record;
	}

	unsigned long long scratchPad;
	if (sscanf(GetFieldValue(0),"%llu", &scratchPad) < 1)
		throw Exception(Debug::major, __LINE__, __FILE__);

	record.moteEui = scratchPad;

	if (sscanf(GetFieldValue(1),"%llu", &scratchPad) < 1)
		throw Exception(Debug::major, __LINE__, __FILE__);

	record.appEui = scratchPad;

	record.applicationKey = GetFieldValue(2);

	return record;
}


bool LoRa::DatabaseAS::WriteJoinMote(EuiType moteEui, EuiType appEui, LoRa::CypherKey const& applicationKey)
{
	std::stringstream query;
	std::string applicationKeyText = applicationKey.CastToString('\0');
	query << "REPLACE INTO joinmotes (eui, appeui, appkey) VALUES (" << moteEui << ", " << appEui << ", \"" << applicationKeyText << "\")";

	return db.Query(query);
}

bool LoRa::DatabaseAS::MoteNonceKnown(EuiType moteEui, uint16 deviceNonce)
{
	//  WARNING returning true is a bad thing - it means that the nonce cannot be used this time

	std::stringstream findQuery;

	findQuery << "SELECT mote FROM nonces WHERE HEX(mote) = \'" << std::hex << moteEui << "\' AND HEX(nonce) = \'" << deviceNonce << '\'';

	SqlDb::Client client(db);
	try
	{
		client.Query(findQuery, true);
	}
	catch (SqlDb::Exception const&)
	{
		return true;
	}
	if (client.RowsInResult() > 0)
		return true;	//already exists

	std::stringstream insertQuery;
	insertQuery << "INSERT INTO nonces (mote, nonce) VALUE (" << moteEui << ", " << deviceNonce << ")";

	db.Query(insertQuery);

	return false;
}

bool LoRa::DatabaseAS::DeleteJoinMote(EuiType moteEui)
{
	std::stringstream query;
	query << "DELETE FROM joinmotes WHERE HEX(eui) = \'" << std::hex << moteEui << '\'';

	return db.Query(query);
}


bool LoRa::DatabaseAS::JoinMoteExists(EuiType moteEui)
{
	std::stringstream findQuery;

	findQuery << "SELECT eui FROM joinmotes WHERE HEX(eui) = \'" << std::hex << moteEui << '\'';

	SqlDb::Client client(db);
	try
	{
		client.Query(findQuery, true);
	}
	catch (SqlDb::Exception const&)
	{
		return false;
	}

	return (client.RowsInResult() > 0);
}


bool LoRa::DatabaseAS::CreateActiveMote(EuiType moteEui, EuiType appEui, LoRa::CypherKey const& sessionKey, uint32 networkAddress)
{
	std::stringstream query;
	std::string sessionKeyText = sessionKey.CastToString('\0');
	query << "INSERT INTO activemotes (eui, appEui, sessionKey, networkAddress) VALUES (" << moteEui << ", " << appEui << ", \"" << sessionKeyText << "\", " << networkAddress << ')';

	return db.Query(query);
}


bool LoRa::DatabaseAS::DeleteActiveMote(EuiType moteEui)
{
	std::stringstream query;
	query << "DELETE FROM activemotes WHERE HEX(eui) = \'" << std::hex << moteEui << '\'';

	return db.Query(query);
}


bool LoRa::DatabaseAS::FindJoinMoteApplicationKey(EuiType moteEui, EuiType appEui, LoRa::CypherKey& applicationKey)
{
	std::stringstream findQuery;
	findQuery << "SELECT appkey FROM joinmotes WHERE HEX(eui) = \'" << std::hex << moteEui << "\' AND HEX(appeui) = \'" << std::hex << appEui << '\'';

	SqlDb::Client client(db);
	try
	{
		client.Query(findQuery, true);
	}
	catch (SqlDb::Exception const&)
	{
		return false;
	}

	if (!client.GetNextRow())
		return false;

	applicationKey = client.GetFieldValue(0);
	return true;
}


void LoRa::DatabaseAS::DeleteAllNoncesBelongingToMote(EuiType moteEui)
{
	std::stringstream query;
	query << "DELETE FROM nonces WHERE HEX(mote) = \'" << std::hex << moteEui << '\'';

	db.Query(query);
}

