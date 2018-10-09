/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "MoteTransmitController.hpp"
#include "MoteNS.hpp"
#include "GatewayMessageProtocol.hpp"
#include "GlobalDataNS.hpp"

#include <sstream>

void MoteTransmitController::ReceivedFrame(LoRa::FrameCopyType frameCopyType, LoRa::ReceivedFrame const& frame)
{
	bestGateway.ReceivedFrame(frameCopyType, frame.gatewayReceiveTime, frame.gatewayReceiveTimestamp_us, frame.signalToNoiseRatio_cB, frame.signalStrength_cBm, frame.GatewayEui(), frame.channel,
		frame.rfChain, frame.frequency_Hz, 
		frame.dataRate, frame.codeRate, frame.AdrEnabled());	//submit frame to bestGateway controller to keep it up to date

	if (frame.Ack())
		waitingFrame.AckReceived();

	//lint --e{1786}  Info -- Implicit conversion to Boolean (assignment)
	moteRequestingResponse |= frame.RequestingResponse();
	moteRequestingAcknowledgement |= frame.IsDataConfirmedFrame();

	if (frame.OptionLength() > 0)
		ReceiveOption(frameCopyType, frame, frame.OptionStart(), frame.OptionLength());
}


bool MoteTransmitController::IsFrameReadyToTransmit() const
{
	if (moteRequestingResponse || moteRequestingAcknowledgement)
		return true;

	if (waitingFrame.Active())
		return true;

	if (waitingApplicationData.Valid())
		return true;

	if (linkCheckController.Active())
		return true;

	if (waitingMacCommands.Active())
		return true;

	return false;
}

uint8 MoteTransmitController::QueueLength(bool app) const
{
	uint8 result = 0;

	if (app)
	{
		if (waitingApplicationData.Valid())
			result++;
	}
	else
	{
		if (linkCheckController.Active())
			result++;
	}
	return result;
}


void MoteTransmitController::GenerateDataFrame()
{
	//decide what to send
	sint32 availableDataBytes = LoRa::maxDownstreamDataBytes;	//signed to avoid underflow later
	sint32 availableOptionBytes  = availableDataBytes - waitingApplicationData.Length();	//signed to avoid underflow later

	if (availableOptionBytes > LoRa::maxHeaderOptionLength)
		availableOptionBytes = LoRa::maxHeaderOptionLength;

	bool transmitApplicationData = waitingApplicationData.Valid();
	uint32 sequenceNumber;
	
	if (transmitApplicationData)
		sequenceNumber = waitingApplicationData.SequenceNumber();
	else
		sequenceNumber = GetNextDownstreamSequenceNumber();

	waitingFrame.SetToJoinAcceptFrame(false);
	waitingFrame.ResetTransmissionCounter();

	// Write header
	waitingFrame.IncreaseLength(1);	//skip MHRD byte
	waitingFrame.Append4ByteValue(mote.NetworkAddress());

	waitingFrame.IncreaseLength(1); //skip FCtrl byte

	waitingFrame.Append2ByteValue(sequenceNumber);

	bool requestFrameAcknowledgement = false;
	//Options
	//send link check if it is requested
	if (linkCheckController.Active())
	{
		LoRa::OptionRecord const& option = linkCheckController.GetCommand();

		waitingFrame.AppendData(option.Data(), option.Length());
		availableOptionBytes -= option.Length();
		availableDataBytes -= option.Length();
		requestFrameAcknowledgement = true;
	}

	if (deviceStatusRequester.Active())
	{
		LoRa::OptionRecord const& option = deviceStatusRequester.GetCommand();

		waitingFrame.AppendData(option.Data(), option.Length());
		availableOptionBytes -= option.Length();
		availableDataBytes -= option.Length();
		requestFrameAcknowledgement = true;
	}

	while (waitingMacCommands.Active() && (waitingMacCommands.Next().option.Length()) <= availableOptionBytes)
	{
		MacCommandRecord const& record = waitingMacCommands.GetNext();	//removes record from list

		waitingFrame.AppendData(record.option.Data(), record.option.Length());

		availableOptionBytes -= record.option.Length();

		if (record.token.Valid())
		{
			waitingFrame.GetTokenList().Add(false, record.token.Value());

			if (record.token.Valid())
				mote.InformUpstreamServerOfTransmissionToMote(false, record.token.Value());
		}
		requestFrameAcknowledgement = true;
	}

	uint8 optionLength = uint8(LoRa::maxHeaderOptionLength - availableOptionBytes);

	// add data
	if (transmitApplicationData)
	{
		waitingFrame.AppendByte(waitingApplicationData.Port());

		//lint --e{514}  (Warning -- Unusual use of a Boolean expression)}
		//lint --e{1786}  (Info -- Implicit conversion to Boolean (assignment) (int to bool))
		requestFrameAcknowledgement |= waitingApplicationData.AcknowledgeRequested();

		waitingFrame.AppendData(waitingApplicationData.Data(), waitingApplicationData.Length());
	}

	//Update header
	LoRa::FrameType frameType = requestFrameAcknowledgement ? LoRa::dataConfirmedFrameDown : LoRa::dataUnconfirmedFrameDown;

	waitingFrame[LoRa::Frame::macHeaderOffset] = uint8(frameType) << LoRa::Frame::typeShift;
	waitingFrame[LoRa::Frame::controlOffset] = uint8(moteRequestingAcknowledgement ? LoRa::Frame::ackMask : 0) | (optionLength & LoRa::headerOptionLengthMask);
	moteRequestingAcknowledgement = false;
	moteRequestingResponse = false;


	LoRa::GenerateDataFrameIntegrityCode(mote.NetworkSessionKey(), waitingFrame.Data(), waitingFrame.Length(), 
		mote.NetworkAddress(), false, sequenceNumber, waitingFrame.FirstUnusedByte());
	waitingFrame.IncreaseLength(LoRa::micBytes);

	if (transmitApplicationData)
	{
		waitingFrame.AddToken(true, waitingApplicationData.Token());

		waitingApplicationData.Clear();
	}
}


void MoteTransmitController::TransmitFrame(EuiType gatewayEui, uint8 transmissionWindow)
{
	sockaddr_in gatewayIpAddress = Global::gatewayList.FindPullIpAddress(gatewayEui);

	if (!IsValid(gatewayIpAddress))
		return;		//gateway pull port is not known so can't transmit

	bool isARepeatTransmission = waitingFrame.Active() && !waitingFrame.First();

	if (Debug::Print(Debug::verbose))
	{
		std::stringstream logText;
		
		logText << "Frame transmitted to " << AddressText(mote.Id()) << " via GW " << AddressText(gatewayEui) << " (" << AddressText(gatewayIpAddress) << 
			")  Time " << GetMsSinceStart();

		if (isARepeatTransmission)
			logText << " REPEAT";

		Debug::Write(logText);
	}

	// waitingApplicationData is cleared by GenerateDataFrame();
	if (!waitingFrame.Active())
	{
		GenerateDataFrame();

		if (!waitingFrame.Active())
		{
			if (Debug::Print(Debug::minor))
				Debug::Write("Unable to create frame for transmission to Mote");
			return;
		}
	}

	if (!isARepeatTransmission && Debug::log.Print(Debug::verbose))
	{
		std::stringstream logText;

		logText << ConvertBinaryArrayToHexTextBlock("Transmitted Frame data", waitingFrame.Data(), waitingFrame.Length());
		logText << std::endl;

		Debug::Write(logText);
	}

	Gateway* gateway = Global::gatewayList.GetById(gatewayEui);

	if (!gateway)
		return;

	bool transmitted = gateway->TransmitFrame(waitingFrame, 
		bestGateway.Receive().receiveTimestamp_us, bestGateway.Transmit(), transmissionWindow, isARepeatTransmission);
	gateway->Unlock();

	if (transmitted)
		waitingFrame.Transmitted();
}


void MoteTransmitController::ReceiveOption(LoRa::FrameCopyType /*frameCopyType*/, LoRa::ReceivedFrame const& frame, uint8 const options[], uint16 length)
{
	uint8 const* ptr = options;
	uint8 const* const end = ptr + length;

	for(uint8 increment= 1; ptr < end; ptr += increment)
	{
		increment= 1;

		switch (*ptr)
		{
		case LoRa::linkCheck:
			linkCheckController.CommandReceived(frame.SequenceNumber(), frame.signalToNoiseRatio_cB.Value() + LoRa::snrMargin_cB);
			break;

		case LoRa::linkAdr:
			increment += 1;
			break;	//no need for acknowledgement

		case LoRa::devStatus:
			increment += 2;
			
			if (ptr < (end - increment))
				mote.Status(ptr[1], ptr[2]);
			break;

		default:	// unrecognised
			return;
		}
	}
}


bool MoteTransmitController::SendApplicationData(uint32 sequenceNumber, LoRa::FrameApplicationData const& data, ValidValueUint16 const& token)
{
	if (waitingApplicationData.Valid())
		return false;

	if ((data.Length() > LoRa::maxDownstreamDataBytes) || (data.Length() == 0))
		return false;

	waitingApplicationData.Set(sequenceNumber, data, token);
	return true;
}


