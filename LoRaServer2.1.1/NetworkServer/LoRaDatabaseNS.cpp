/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "LoRaDatabaseNS.hpp"
#include "DebugMonitor.hpp"
#include "Utilities.hpp"
#include <sstream>
#include <iomanip>

LoRa::DatabaseNS::MoteRecord LoRa::DatabaseNS::MoteClient::Read()
{
	LoRa::DatabaseNS::MoteRecord record;

	if (!GetNextRow())
	{
		record.moteEui = invalidEui;
		return record;
	}

	record.moteEui = ReadUnsignedLongInteger(GetFieldValue(0));
	record.appEui =  ReadUnsignedLongInteger(GetFieldValue(1));
	record.networkAddress =  ReadUnsignedInteger(GetFieldValue(2));
	record.networkSessionKey = GetFieldValue(3);
	record.downMessageSequenceNumber = ReadUnsignedInteger(GetFieldValue(4));
	record.upMessageSequenceNumber = ReadUnsignedInteger(GetFieldValue(5));

	return record;
}


bool LoRa::DatabaseNS::CreateMote(EuiType moteEui, EuiType appEui, uint32 networkAddress, LoRa::CypherKey const& networkSessionKey, uint32 upMessageSequenceNumber, uint32 downMessageSequenceNumber)
{
	std::stringstream query;

	query << "INSERT INTO motes (eui, appeui, networkAddress, networkSessionKey, upMsgSeqNo, downMsgSeqNo) VALUES (" <<
		moteEui << ", " <<
		appEui << ", " << 
		networkAddress << ", "<<
		'\"' << networkSessionKey.CastToString('\0') << "\", " <<
		upMessageSequenceNumber << ", " <<
		downMessageSequenceNumber << ")";

	return db.Query(query);
}

bool LoRa::DatabaseNS::DeleteMote(EuiType moteEui)
{
	std::stringstream query;

	query << "DELETE FROM motes WHERE HEX(eui) = \'" << std::hex << moteEui << '\'';

	return db.Query(query);
}

bool LoRa::DatabaseNS::UpdateMoteSequenceNumbers(EuiType moteEui, uint32 downstreamSequenceNumber, uint32 upstreamSequenceNumber)
{
	std::stringstream query;

	query << "UPDATE motes SET downMsgSeqNo = " << downstreamSequenceNumber << ", upMsgSeqNo = " << upstreamSequenceNumber << " WHERE eui = " << moteEui;

	return db.Query(query);
}


bool LoRa::DatabaseNS::CreateGateway(EuiType eui, LoRa::Region region, bool allowGpsToSetPosition)
{
	std::stringstream query;

	query << "INSERT INTO gateways (eui, region, allowGpsToSetPosition) VALUE (" << eui << ", " << region << ", " << (allowGpsToSetPosition ? "TRUE" : "FALSE") << ")";

	return db.Query(query);
}


bool LoRa::DatabaseNS::UpdateGateway(EuiType eui, LoRa::Region region)
{
	std::stringstream query;
	query << "UPDATE gateways SET region = " << region << " WHERE eui = " << eui;

	return db.Query(query);
}


bool LoRa::DatabaseNS::UpdateGateway(EuiType eui, GatewayFrameCountType const& count)
{
	std::stringstream query;
	bool upStreamDatagramsAcknowledgedRatioIsValid = count.upstreamPacketsReceived.Valid() && count.upStreamDatagramsAcknowledged.Valid();

	uint termsWritten = 0;

	query << "UPDATE gateways SET";

	if (count.downstreamDatagramsReceived.Valid())
		query << (termsWritten++ ? ',':' ') << " downpacketsreceived = " << count.downstreamDatagramsReceived.Value();

	if (count.upstreamGoodPacketsReceived.Valid())
		query << (termsWritten++ ? ',':' ') << " gooduppacketsreceived = " << count.upstreamGoodPacketsReceived.Value();

	if (count.packetsTransmitted.Valid())
		query << (termsWritten++ ? ',':' ') << " packetstransmitted = " << count.packetsTransmitted.Value();

	if (upStreamDatagramsAcknowledgedRatioIsValid)
	{
		float upStreamDatagramsAcknowledgedRatio;

		if (count.upstreamPacketsReceived.Value() > 0)
			upStreamDatagramsAcknowledgedRatio = float(count.upStreamDatagramsAcknowledged.Value()) / count.upstreamPacketsReceived.Value();
		else
			upStreamDatagramsAcknowledgedRatio = 0;

		query << (termsWritten++ ? ',':' ') << " uppacketsacknowedgedratio = " << upStreamDatagramsAcknowledgedRatio;
	}

	if (count.upstreamPacketsForwarded.Valid())
		query << (termsWritten++ ? ',':' ') << " uppacketsforwarded = " << count.upstreamPacketsForwarded.Value();

	if (count.upstreamPacketsReceived.Valid())
		query << (termsWritten++ ? ',':' ') << " uppacketsreceived = " << count.upstreamPacketsReceived.Value();

	if (termsWritten == 0)
		return true;

	query << " WHERE eui = " << eui;

	return db.Query(query);
}


bool LoRa::DatabaseNS::UpdateGateway(EuiType eui, TimeRecord const& time)
{
	if (!time.Valid())
		return true;

	std::stringstream query;

	query << "UPDATE gateways SET time = \'" << TimeRecord::SQLString(time) << "\' WHERE eui = " << eui;

	return db.Query(query);
}


bool LoRa::DatabaseNS::UpdateGateway(EuiType eui, Position const& position)
{
	if (!position.Valid())
		return true;

	std::stringstream query;

	query << "UPDATE gateways SET " << 
		"latitude = " << position.latitude.Value() << 
		", longitude = " << position.longitude.Value();

	if (position.altitude.Valid())
			query << ", altitude = " << position.altitude.Value();

	query << " WHERE eui = " << eui;

	return db.Query(query);
}


bool LoRa::DatabaseNS::UpdateGatewayAllowGpsToSetPosition(EuiType eui, bool allowGpsToSetPosition)
{
	std::stringstream query;

	query << "UPDATE gateways SET allowGpsToSetPosition = " << (allowGpsToSetPosition ? "TRUE" : "FALSE") << " WHERE eui = " << eui;

	return db.Query(query);
}


bool LoRa::DatabaseNS::DeleteGateway(EuiType eui)
{
	std::stringstream query;

	query << "DELETE FROM gateways WHERE HEX(eui) = \'" << std::hex << eui << '\'';

	return db.Query(query);
}


LoRa::DatabaseNS::GatewayRecord LoRa::DatabaseNS::GatewayClient::Read()
{
	LoRa::DatabaseNS::GatewayRecord record;

	if (!GetNextRow())
	{
		record.eui = invalidEui;
		return record;
	}

	record.eui = ReadUnsignedLongInteger(GetFieldValue(0));
	record.allowGpsToSetPosition = ReadBoolean(GetFieldValue(1)) != 0 ? true : false;	//default to true
	
	record.region = LoRa::Region(ReadUnsignedInteger(GetFieldValue(2)));

	if (record.region == -1 || record.region >= LoRa::numberOfRegions)
		record.region = LoRa::numberOfRegions;

	double scratchPad[3];	//3 position parameters

	uint i = 0;
	for (; i < 3; i++)
	{
		if (!ReadRealNumber(GetFieldValue(3+i), scratchPad[i]))
			break;
	}

	bool positionRead = i >= 2;
	bool altitudeRead = i >= 3;

	if (positionRead)
	{
		record.position.latitude = scratchPad[0];
		record.position.longitude = scratchPad[1];

		if (altitudeRead)
			record.position.altitude = scratchPad[2];
	}

	record.frameCount.upstreamPacketsReceived = ReadUnsignedInteger(GetFieldValue(6));
	record.frameCount.upstreamGoodPacketsReceived = ReadUnsignedInteger(GetFieldValue(7));
	record.frameCount.upstreamPacketsForwarded = ReadUnsignedInteger(GetFieldValue(8));
	record.frameCount.upStreamDatagramsAcknowledged = ReadUnsignedInteger(GetFieldValue(9));
	record.frameCount.downstreamDatagramsReceived = ReadUnsignedInteger(GetFieldValue(10));
	record.frameCount.packetsTransmitted = ReadUnsignedInteger(GetFieldValue(11));

	return record;
}


bool LoRa::DatabaseNS::UpdateDBSpecificStructure()
{
	bool result = true;

	result &= AddGatewayAllowGPSToSetPositionColumn();
	result &= AddGatewayRegionColumn();

	return result;
}

