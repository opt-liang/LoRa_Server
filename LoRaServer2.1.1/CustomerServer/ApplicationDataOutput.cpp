/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "ApplicationDataOutput.hpp"
#include "GlobalDataCS.hpp"
#include "JsonCommand.hpp"

#include <sstream>

void ApplicationDataOutput::UpstreamApplicationDataReceived(EuiType moteEui, uint32 sequenceNumber, 
		LoRa::FrameApplicationData const& payload, 
		MoteTransmitRecord const& transmitRecord, 
		GatewayReceiveList const& gatewayReceiveList)
{
	TimeRecord time = gatewayReceiveList.ReceiveTime();

	if (!time.Valid())
		time.SetToSystemTime();

	if (writeToSqlDatabase)
		WriteToDatabase(moteEui, time, sequenceNumber, payload, transmitRecord, gatewayReceiveList);

	if (writeToOutputFile && OutputFilesOpen())
		WriteToOutputFiles(moteEui, time, sequenceNumber, payload, transmitRecord, gatewayReceiveList);
}

void ApplicationDataOutput::PositionResultReceived(EuiType mote, Position const& position)
{
	if (writeToOutputFile && OutputFilesOpen())
		WriteToFile(mote, position);
}

void ApplicationDataOutput::WriteToDatabase(EuiType moteEui, TimeRecord const& time, uint32 sequenceNumber, 
		LoRa::FrameApplicationData const& payload, 
		MoteTransmitRecord const& transmitRecord, 
		GatewayReceiveList const& gatewayReceiveList)
{
	uint64 recordId = Global::customerDatabase.AddApplicationData(moteEui, time, sequenceNumber, payload.Port(), payload.Data(), payload.Length());

	if (transmitRecord.Valid())
		Global::customerDatabase.AddMoteTransmissionRecord(recordId, 
			transmitRecord.frequency_Hz, 
			transmitRecord.dataRate, 
			transmitRecord.codeRate, 
			transmitRecord.adrEnabled);

	gatewayReceiveList.Lock();
	GatewayReceiveList::List::const_iterator it = gatewayReceiveList.IteratorBegin();

	for (uint count = 0; it != gatewayReceiveList.IteratorEnd(); it++, count++)
	{
		Global::customerDatabase.SetGatewayLastFrameId((*it)->Eui(), recordId);
		Global::customerDatabase.AddGatewayReceptionRecord(recordId, count, 
			(*it)->Eui(), 
			(*it)->receiveTime, 
			(*it)->channel, 
			(*it)->rfChain,
			(*it)->signalToNoiseRatio_cB, 
			(*it)->signalStrength_cBm);
	}

	Global::customerDatabase.SetMoteMostRecentlyReceivedFrame(moteEui, recordId);
	gatewayReceiveList.Unlock();
}

void ApplicationDataOutput::WriteToOutputFiles(EuiType moteEui, TimeRecord const& time, uint32 sequenceNumber, 
	LoRa::FrameApplicationData const& payload, 
	MoteTransmitRecord const& transmitRecord, 
	GatewayReceiveList const& gatewayReceiveList)
{
	WriteToFile(moteEui, sequenceNumber, time, payload);

	if (transmitRecord.Valid())
		WriteToFile(moteEui, sequenceNumber, time, transmitRecord);

	gatewayReceiveList.Lock();

	GatewayReceiveList::List::const_iterator it = gatewayReceiveList.IteratorBegin();

	for (uint count = 0; it != gatewayReceiveList.IteratorEnd(); it++, count++)
	{
		GatewayReceiveRecord const* record = *it;

		WriteToFile(moteEui, sequenceNumber, *record);
	}
	gatewayReceiveList.Unlock();
	appDataCount++;
}

void ApplicationDataOutput::WriteToFile(EuiType mote, uint32 sequenceNumber, TimeRecord const& time, LoRa::FrameApplicationData const& payload)
{
	appDataFile << 
		AddressText(mote) << '\t' << 
		time.SQLString() << '\t' << 
		appDataCount << '\t' <<
		sequenceNumber << '\t' << 
		uint(payload.Port()) << '\t' << 
		ConvertBinaryArrayToHexText(payload.Data(), payload.Length(), true,' ') << 
		std::endl;
}

void ApplicationDataOutput::WriteToFile(EuiType mote, uint32 sequenceNumber, TimeRecord const& time, MoteTransmitRecord const& t)
{
	moteTxFile << 
		AddressText(mote) << '\t' << 
		time.SQLString() << '\t' << 
		appDataCount << '\t' <<
		sequenceNumber << '\t' <<
		t.frequency_Hz << '\t' <<
		(t.adrEnabled ? "on" : "off") << '\t' <<
		((t.dataRate.modulation == LoRa::loRaMod) ? "LoRa" : "FSK") << '\t';

		if (t.dataRate.modulation == LoRa::loRaMod)
			moteTxFile << std::string(t.codeRate) << '\t' << std::string(t.dataRate);
		else
			moteTxFile << t.dataRate.bandwidth_Hz;

	moteTxFile << std::endl;
}

void ApplicationDataOutput::WriteToFile(EuiType mote, uint32 sequenceNumber, GatewayReceiveRecord const& r)
{
	gatewayRxFile << 
		AddressText(mote) << '\t' << 
		r.receiveTime.SQLString() << '\t' << 
		appDataCount << '\t' <<
		sequenceNumber << '\t' <<
		(float(r.signalToNoiseRatio_cB) / 10) << '\t' <<
		(float(r.signalStrength_cBm) / 10) << '\t' << 
		std::endl;
}

void ApplicationDataOutput::WriteToFile(EuiType mote, const Position& position)
{
	//position.latitude and position.longitude must be valid when function is called

	motePositionFile << 
		AddressText(mote) << '\t' << 
		position.latitude << '\t' << 
		position.longitude << '\t';

	if (position.altitude.Valid())
		motePositionFile << position.altitude;
	
	motePositionFile << '\t';
	
	if (position.toleranceHorizonal.Valid())
		motePositionFile << position.toleranceHorizonal;
	
	motePositionFile << '\t';

	if (position.toleranceVertical.Valid())
		motePositionFile << position.toleranceVertical;
	
	motePositionFile << '\t';
	
	motePositionFile << std::endl;
}


bool ApplicationDataOutput::SetOutputFile(std::string const& name)
{
	if (outputFileName != name)
	{
		outputFileName = name;
		std::string appDataFileName = name + ".appData.txt";
		std::string moteTxFileName = name + ".motetx.txt";
		std::string gatewayRxFileName = name + ".gatewayrx.txt";
		std::string motePositionFileName = name + ".motePosition.txt";

		appDataFile.open(appDataFileName.c_str(), std::ios_base::out | std::ios_base::ate);
		moteTxFile.open(moteTxFileName.c_str(), std::ios_base::out | std::ios_base::ate);
		gatewayRxFile.open(gatewayRxFileName.c_str(), std::ios_base::out | std::ios_base::ate);
		motePositionFile.open(motePositionFileName.c_str(), std::ios_base::out | std::ios_base::ate);
	}
	return OutputFilesGood();
}

bool ApplicationDataOutput::EnableFileWrite(bool on)
{
	if (on)
	{
		if (!OutputFilesOpen())
			return false;
	}
	writeToOutputFile = on;
	return true;
}

void ApplicationDataOutput::CloseOutputFiles()
{
	if (appDataFile.is_open())		appDataFile.close();
	if (moteTxFile.is_open())		moteTxFile.close();
	if (gatewayRxFile.is_open())	gatewayRxFile.close();
	if (motePositionFile.is_open())	motePositionFile.close();
}

