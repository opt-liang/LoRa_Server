/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef ADAPTIVE_DATA_RATE_CONTROLLER_HPP
#define ADAPTIVE_DATA_RATE_CONTROLLER_HPP

#include "General.h"
#include "LoRa.hpp"
#include "MonitorAlgorithm.hpp"
#include "DataRateStore.hpp"

class AdaptiveDataRateController : public MonitorAlgorithm
{
private:
	static const uint16			spreadingFactorValuesUsed = 11;
	static const sint16			spreadingFactorMargin_cB = 100;
	static const uint16			minAdrChangeCommandSendPeriodInFrames = 4;	//min gap between SF change (either direction) commands (in units of frames)
	static const uint16			minAdrIncreaseCommandSendPeriodInFrames = 2;	//min gap between SF increase commands (in units of frames)
	static const uint8			motePower = 1; //14dBm in Europe and 28dBm in Americas

	LoRa::DataRate				rxDataRate;
	uint16						framesReceivedSinceLastSend;
	DataRateStore				dataRateStore;

public:
	AdaptiveDataRateController(Mote& myMote, uint16 myAlgorithmNumber, bool myActive)
		: MonitorAlgorithm(myMote, myAlgorithmNumber, myActive),
		framesReceivedSinceLastSend(0),
		dataRateStore(spreadingFactorValuesUsed, spreadingFactorMargin_cB)
	{}

	//redefined virtual functions from MonitorAlgorithm
	void NewData(uint32 sequenceNumber, MoteTransmitRecord const& transmitRecord, GatewayReceiveList const& gatewayReceiveList);
	char const* AlgorithmName() const {return "adr";}
	bool CommandReceived(WordStore& commandWords);
	//end of redefined virtual functions

	LoRa::DataRate GetIdealDataRate() const {return dataRateStore.GetIdealDataRate();}
	uint8 GetMacCommandDataRate() const;

private:
	bool ValidData() const {return dataRateStore.Valid() && rxDataRate.Valid();}
	bool RequestingDRChange() const {return ValidData() && (dataRateStore.GetIdealDataRate() != rxDataRate);}
	bool RequestingDRDecrease() const {return ValidData() && (dataRateStore.GetIdealDataRate() > rxDataRate);}
	bool ReadyToSend() const {return (RequestingDRChange() && framesReceivedSinceLastSend > minAdrChangeCommandSendPeriodInFrames) 
		|| (RequestingDRDecrease() && framesReceivedSinceLastSend > minAdrIncreaseCommandSendPeriodInFrames);}

	void SendAdrMacCommand();
	void SendAdrMacCommand(uint8 power, uint8 dataRate, uint16 channelMask, uint8 channelMaskControl, uint8 transmissionsOfUnacknowledgedUplinkFrame);
	void SendAdrMacCommand(LoRa::OptionRecord const& adrCommand);
};

#endif
