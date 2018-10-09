/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "LoRaDatabaseCS.hpp"


struct LoRa::DatabaseCS::MoteRecord LoRa::DatabaseCS::MoteClient::Read(void)
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


bool LoRa::DatabaseCS::CreateMote(EuiType moteEui, EuiType appEui)
{
	std::stringstream query;

	query << "INSERT INTO motes (eui, appEui) VALUES (" << moteEui << ", " << appEui << ')';

	return db.Query(query);
}


bool LoRa::DatabaseCS::DeleteMote(EuiType moteEui)
{
	std::stringstream query;
	query << "DELETE FROM motes WHERE HEX(eui) = \'" << std::hex << moteEui << '\'';

	return db.Query(query);
}


uint64 LoRa::DatabaseCS::AddApplicationData(EuiType moteEui, TimeRecord const& time, uint32 seqno, uint8 port, uint8 const data[], uint16 length)
{
	std::string dataString = ConvertBinaryArrayToHexText(data, length, true);

	std::stringstream query;

	query << "INSERT INTO appdata (mote, ";
	
	if (time.Valid())
		query << "time, time_usec, accurateTime, ";
	
	query << "seqNo, port, data) VALUES (" <<
		moteEui << ", ";

	if (time.Valid())
		query << "\"" << time.SQLString() << "\", " << time.USec() << ", " << (time.Accurate() ? 1 : 0) << ", ";

	query <<
		seqno << ", " <<
		uint(port) << ", \"" <<
		dataString << "\"" <<
		")";

	return db.QueryReportingAutoIncrementValue(query);
}


bool LoRa::DatabaseCS::AddMoteTransmissionRecord(uint64 databaseId, uint32 frequency_Hz, 
	LoRa::DataRate const& dataRate, LoRa::CodeRate const& codeRate, bool adrEnabled)
{
	std::stringstream query;

	query << "INSERT INTO moteframetx (id, frequency, loraAdrEnabled, loraModulation, ";
	
	if (dataRate.modulation == LoRa::loRaMod)
		query << "loraModulationBandwidth_Hz, loraSpreadingFactor, loraCodingRateNumerator, loraCodingRateDenominator";
	else
		query <<"fskBitRate";
	
	query << ") VALUES (" <<

	databaseId << ", " <<
	frequency_Hz << ", " <<
	(adrEnabled ? 1 : 0) << ", " <<
	(dataRate.modulation == LoRa::loRaMod ? 1 : 0) << ", ";
	
	if (dataRate.modulation == LoRa::loRaMod)
		query << 
			(dataRate.bandwidth_Hz / 1000) << ", " <<
			unsigned(dataRate.spreadingFactor) << ", " <<
			codeRate.carried << ", " <<
			codeRate.total;
	else
		query << dataRate.bandwidth_Hz;
	
	query << ")";

	return db.Query(query);
}


bool LoRa::DatabaseCS::AddGatewayReceptionRecord(uint64 databaseId, uint16 rank, EuiType gatewayEui, 
	TimeRecord const& receiveTime, ValidValueUint16 const& channel, ValidValueUint16 const& rfChain, 
	ValidValueSint16 const& signalToNoiseRatio_cB, ValidValueSint16 const& signalStrength_cBm)
{
	std::stringstream query;

	query << "INSERT INTO gatewayframerx (id, rank, gatewayEui, receiveTime, receiveTime_nsec, receiveTimeAccurate";
	
	if (channel.Valid())
		query << ", channel";
	
	if (rfChain.Valid())
		query << ", rfChain";
	
	if (signalStrength_cBm.Valid())
		query << ", signalStrength_dBm";
	
	if (signalToNoiseRatio_cB.Valid())
		query << ", signalToNoiseRatio_cB";
		
	query << ") VALUES (" <<

	databaseId << ", " <<
	rank << ", " <<
	gatewayEui << ", " <<
	'\'' << receiveTime.SQLString() << "\', " <<
	receiveTime.NSec() << ", " <<
	(receiveTime.Accurate() ? 1 : 0);

	if (channel.Valid())
		query << ", " << channel.Value();
	
	if (rfChain.Valid())
		query << ", " << rfChain.Value();
	
	if (signalStrength_cBm.Valid())
		query << ", " << (signalStrength_cBm.Value() /10);	//stored as dBm
	
	if (signalToNoiseRatio_cB.Valid())
		query << ", " << signalToNoiseRatio_cB.Value();
	
	query << ")";

	return db.Query(query);
}

bool LoRa::DatabaseCS::SetGatewayLastFrameId(EuiType gatewayEui, uint64 frameDatabaseId)
{
	//REPLACE is used because the CS keeps no record (in memory) of gateways seen
	std::stringstream query;

	query << "REPLACE INTO gateways (eui, lastRxFrame) VALUES (" << gatewayEui << ", " << frameDatabaseId << ")";

	return db.Query(query);
}


bool LoRa::DatabaseCS::SetMoteMostRecentlyReceivedFrame(EuiType moteEui, uint64 mostRecentlyReceivedFrameDatabaseId)
{
	std::stringstream query;

	query << "UPDATE motes SET lastRxFrame= " << mostRecentlyReceivedFrameDatabaseId << " WHERE HEX(eui) = \'" << std::hex << moteEui << '\'';

	return db.Query(query);
}


