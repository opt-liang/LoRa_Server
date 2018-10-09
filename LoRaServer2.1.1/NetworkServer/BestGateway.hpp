/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef BEST_GATEWAY_CHANNEL_HPP
#define BEST_GATEWAY_CHANNEL_HPP

#include "General.h"
#include "Eui.hpp"
#include "FrameReception.hpp"


class BestGateway
{
private:
	ReceivedMoteTransmitRecord		transmit;
	GatewayReceiveRecord			bestGateway;
	bool							valid; //only false until first frame received

public:
	BestGateway()
		: valid(false)
	{}

	void ReceivedFrame(LoRa::FrameCopyType frameCopyType, TimeRecord const& receiveTime, uint32 gatewayReceiveTimestamp, 
		ValidValueSint16 const& snr_cB, sint16 signalStrength_cBm,
		EuiType const& gateway, uint16 channel, 
		uint16 rxRfChain, uint32 rxFrequency_Hz, 
		LoRa::DataRate const& rxDataRate,
		LoRa::CodeRate const& rxCodeRate,
		bool adrEnabled)
	{
		bool updateReceiveRecord;
		bool updateTransmitRecord;
		sint16 newSignalQuality_cB = snr_cB + signalStrength_cBm;

		switch (frameCopyType)
		{
		case LoRa::first:
		case LoRa::retransmission:
		case LoRa::resetDetected:
			updateReceiveRecord = true;
			updateTransmitRecord = true;
			break;

		case LoRa::duplicate:
			updateReceiveRecord = bestGateway.SignalQuality_cB() < newSignalQuality_cB;
			updateTransmitRecord = updateReceiveRecord && (transmit.frequency_Hz.Value() != rxFrequency_Hz);
			//Duplicate frames should always have the same frequency - but sometimes a 'ghost' frame is received on an incorrect frequency when the transmitted signal is very strong.
			break;

		default:		//all other values are unacceptable
			return;
		}

		valid = true;
		if (updateReceiveRecord)
			bestGateway.Set(gateway, receiveTime, gatewayReceiveTimestamp, channel, rxRfChain, snr_cB, signalStrength_cBm);

		if (updateTransmitRecord)
			transmit.Update(frameCopyType, gateway, newSignalQuality_cB, rxFrequency_Hz, rxDataRate, rxCodeRate, adrEnabled);
	}

	bool Valid() const {return valid;}
	EuiType Eui() const {return bestGateway.Eui();}
	MoteTransmitRecord const& Transmit() const {return transmit;}
	GatewayReceiveRecord const& Receive() const {return bestGateway;}
};

#endif

