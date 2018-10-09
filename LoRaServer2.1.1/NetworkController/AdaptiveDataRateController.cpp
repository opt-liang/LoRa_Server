/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "AdaptiveDataRateController.hpp"
#include "TransmissionRecord.hpp"
#include "GlobalDataNC.hpp"

void AdaptiveDataRateController::NewData(uint32 sequenceNumber, MoteTransmitRecord const& transmitRecord, GatewayReceiveList const& gatewayReceiveList)
{
	rxDataRate = transmitRecord.dataRate;

	if (transmitRecord.adrEnabled != Active())
		Active(transmitRecord.adrEnabled);

	LoRa::ValidRegion moteRegion = mote.GetRegion();
	if (!moteRegion.Valid())
		return;	// SF range is region dependent

	if (!dataRateStore.GetRegion().Valid() || (dataRateStore.GetRegion().Value() != moteRegion.Value()))
	{
		dataRateStore.EmptyStore();
		dataRateStore.SetRegion(moteRegion);
	}

	if (rxDataRate.IsHighBandwidth())
		return;	//High bandwidth is used only by Americas902 and does not use ADR

	if (!gatewayReceiveList.IsEmpty())
		dataRateStore.Add(sequenceNumber, (*gatewayReceiveList.IteratorBegin())->signalToNoiseRatio_cB);

	if (framesReceivedSinceLastSend < UINT16_MAX)
		framesReceivedSinceLastSend++;

	if (rxDataRate.Modulation() != LoRa::loRaMod)
		return;	// the mote is using FSK modulation

	if (rxDataRate == dataRateStore.GetIdealDataRate())
		return;

	if (!mote.AdrEnabled())
		return;

	if (!ReadyToSend())
		return;

	if (Debug::Print(Debug::monitor))
	{
		std::stringstream text;

		text << "Requested Mote " << AddressText(mote.Id()) << " to use " << std::string(dataRateStore.GetIdealDataRate());
		Debug::Write(text);
	}

	SendAdrMacCommand();
	framesReceivedSinceLastSend = 0;
}


void AdaptiveDataRateController::SendAdrMacCommand()
{
	uint8 dataRate = GetMacCommandDataRate();

	SendAdrMacCommand(motePower, dataRate, Global::defaultMoteChannelMask, uint8(Global::defaultMoteChannelMaskControl), uint8(Global::transmissionsOfUnacknowledgedUplinkFrame));
}


void AdaptiveDataRateController::SendAdrMacCommand(uint8 power, uint8 dataRate, uint16 channelMask, uint8 channelMaskControl, uint8 transmissionsOfUnacknowledgedUplinkFrame)
{
	LoRa::OptionRecord adrCommand = LoRa::GenerateAdrCommand(power, dataRate, channelMask, channelMaskControl, transmissionsOfUnacknowledgedUplinkFrame);

	SendAdrMacCommand(adrCommand);
}


void AdaptiveDataRateController::SendAdrMacCommand(LoRa::OptionRecord const& adrCommand)
{
	JSON::String jsonString;

	jsonString.Open();
	jsonString.AddMacCommand(mote.Id(), invalidValueUint16, adrCommand, false);
	jsonString.Close();

	Global::applicationList.Send(mote.ApplicationEui(), Service::downstreamServer, jsonString);
}


bool AdaptiveDataRateController::CommandReceived(WordStore& commandWords)
{
	std::string const& command = commandWords.GetNextWord();

	if (command == "enable")
	{
		bool enable = commandWords.GetNextBoolean();

		Active(enable);
	}
	else
		return false;

	return true;
}

uint8 AdaptiveDataRateController::GetMacCommandDataRate() const
{
	return LoRa::TranslateDataRateToDRFrameHeaderNibble(dataRateStore.GetRegion(), GetIdealDataRate(), true, false);
}

