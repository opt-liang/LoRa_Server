/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "LoRaDatabaseNC.hpp"

LoRa::DatabaseNC::MoteRecord LoRa::DatabaseNC::MoteClient::Read(void)
{
	MoteRecord record;

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

	return record;
}


bool LoRa::DatabaseNC::CreateMote(EuiType moteEui, EuiType appEui)
{
	std::stringstream query;
	query << "INSERT INTO motes (eui, appEui) VALUES (" << moteEui << ", " << appEui << ")";

	return db.Query(query);
}

bool LoRa::DatabaseNC::DeleteMote(EuiType moteEui)
{
	std::stringstream query;
	query << "DELETE FROM motes WHERE HEX(eui) = \'" << std::hex << moteEui << '\'';

	return db.Query(query);
}

