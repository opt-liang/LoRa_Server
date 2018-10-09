/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef APPLICATION_DATA_OUTPUT_HPP
#define APPLICATION_DATA_OUTPUT_HPP

#include "General.h"
#include "LoRa.hpp"
#include "TimeRecord.hpp"
#include "TransmissionRecord.hpp"
#include "Position.hpp"

#include <string>
#include <fstream>


class ApplicationDataOutput
{
private:
	bool			writeToSqlDatabase;
	bool			writeToOutputFile;
	std::string		outputFileName;
	uint32			appDataCount;	//count of records written

	std::ofstream	appDataFile;
	std::ofstream	moteTxFile;
	std::ofstream	gatewayRxFile;
	std::ofstream	motePositionFile;
	
public:
	ApplicationDataOutput()
		: writeToSqlDatabase(true), writeToOutputFile(false), appDataCount(0)
	{}

	~ApplicationDataOutput()
	{
		CloseOutputFiles();
	}

	void EnableDatabaseWrite(bool on) {writeToSqlDatabase = on;}
	bool EnableFileWrite(bool on);
	bool SetOutputFile(std::string const& name);
	bool FileNameValid() {return !outputFileName.empty();}

	void UpstreamApplicationDataReceived(EuiType mote, uint32 sequenceNumber, 
		LoRa::FrameApplicationData const& payload, 
		MoteTransmitRecord const& transmitRecord, 
		GatewayReceiveList const& gatewayReceiveList);

	void PositionResultReceived(EuiType mote, Position const& position);

private:
	void WriteToDatabase(EuiType moteEui, TimeRecord const& time, uint32 sequenceNumber, 
		LoRa::FrameApplicationData const& payload, 
		MoteTransmitRecord const& transmitRecord, 
		GatewayReceiveList const& gatewayReceiveList);

	void WriteToOutputFiles(EuiType moteEui, TimeRecord const& time, uint32 sequenceNumber, 
		LoRa::FrameApplicationData const& payload, 
		MoteTransmitRecord const& transmitRecord, 
		GatewayReceiveList const& gatewayReceiveList);

	bool OutputFilesOpen() {return appDataFile.is_open() && moteTxFile.is_open() && gatewayRxFile.is_open() && motePositionFile.is_open();}
	bool OutputFilesErrors() {return appDataFile.fail() || moteTxFile.fail() || gatewayRxFile.fail() || motePositionFile.fail();}
	bool OutputFilesGood() {return OutputFilesOpen() && !OutputFilesErrors();}
	void CloseOutputFiles();

	void WriteToFile(EuiType mote, uint32 sequenceNumber, TimeRecord const& time, LoRa::FrameApplicationData const& payload);
	void WriteToFile(EuiType mote, uint32 sequenceNumber, TimeRecord const& time, MoteTransmitRecord const& transmitRecord);
	void WriteToFile(EuiType mote, uint32 sequenceNumber, GatewayReceiveRecord const& gatewayReceiveRecord);
	void WriteToFile(EuiType mote, Position const& position);
};

#endif

