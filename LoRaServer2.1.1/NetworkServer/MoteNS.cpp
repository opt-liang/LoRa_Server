/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "MoteNS.hpp"
#include "Utilities.hpp"
#include "GlobalDataNS.hpp"
#include "DebugMonitor.hpp"
#include "TimeRecord.hpp"
#include "JsonGenerate.hpp"

#include <iostream>

bool MoteList::DeleteById(EuiType eui)
{
	Mote* mote = GetById(eui);

	if (!mote)
		return false;

	Global::joinController.ReleaseNetworkAddress(mote->NetworkAddress());

	mote->Unlock();

	return BinarySearchVectorNV::List<Mote, EuiType>::DeleteById(eui);
}


void Mote::ReceivedDataFrame(LoRa::ReceivedFrame& frame)
{
	LoRa::FrameCopyType frameCopyType = AuthenticateAndAcceptDataFrame(frame);

	if (frameCopyType == LoRa::failedAuthentication || frameCopyType == LoRa::invalidDelay)
	{
		if (Debug::Print(Debug::verbose))
			Debug::Write(FrameCopyTypeText(frameCopyType));

		return;
	}

	TimeRecord frameReceiveTime;
	if (frame.gatewayReceiveTime.Valid())
		frameReceiveTime = frame.gatewayReceiveTime;
	else
		frameReceiveTime.SetToSystemTime();

	transmitController.ReceivedFrame(frameCopyType, frame);

	uint16 payloadLength = frame.PayloadLength();

	uint64 forwardTime_ms = frame.ThisServerReceiveTime_ms() + Global::gatewayToNetworkServerMaxDelay_ms;
	Global::frameReceptionList.Store(frameCopyType, Id(), frame, forwardTime_ms, frameReceiveTime);

	if (frame.OptionLength() > 0)
	{
		switch(frameCopyType)
		{
		case LoRa::first:
		case LoRa::retransmission:
		case LoRa::resetDetected:
			if (frame.OptionLength() > 0)
				ForwardHeaderOptions(frame);
			break;

		default:
			break;
		}
	}
	if (Debug::Print(Debug::verbose))
	{
		std::stringstream logText;
		logText << LoRa::FrameCopyTypeText(frameCopyType) << " frame accepted from Mote " << AddressText(Id()) << "  (App EUI " << AddressText(ApplicationEui()) << ")  " <<
			"GW " << AddressText(frame.GatewayEui()) << "  ("<< frame.GatewayAddress().Text() << ")  " << 
			"Seq# " << std::hex << frameCopyArbitrator.LastUpMessageSequenceNumber() << "  ADR " << (frame.AdrEnabled() ? "enabled  " : "disabled  ") << 
			"Datarate " << std::string(frame.dataRate) << "  " <<
			"Time " << std::dec << frame.ThisServerReceiveTime_ms();

		Debug::Write(logText);
	}

	if (frameCopyType != LoRa::duplicate)
	{
		//queue transmit opportunity (for transmission to mote)
		uint8 transmissionWindow = uint8(Global::moteReceiveWindow);
		uint64 serverTxTime_ms = frame.ThisServerReceiveTime_ms() + 
			uint64(LoRa::moteWindowTime_us[transmissionWindow]) / 1000 - 
			(Global::gatewayToNetworkServerMaxDelay_ms + Global::networkServerToGatewayMaxDelay_ms) - Global::timeThreadTickPeriod_ms;

		Global::transmitQueue.Add(Id(), serverTxTime_ms, transmissionWindow);
	}

	UpdateNVStoreSequenceNumber();
}


LoRa::FrameCopyType Mote::AuthenticateAndAcceptDataFrame(LoRa::ReceivedFrame& frame)
{
	uint32 inferredSequenceNumber;
	LoRa::FrameCopyType frameCopyType = AuthenticateDataFrame(frame, inferredSequenceNumber);

	switch(frameCopyType)
	{
	case LoRa::first:
	case LoRa::retransmission:
		frameCopyArbitrator.AcceptAsFirstCopy(inferredSequenceNumber, frame.ThisServerReceiveTime_ms());
		break;

	case LoRa::failedAuthentication:
		authenticationFailsSinceMostRecentAcceptedFrame++;
		return frameCopyType;

	case LoRa::invalidDelay:
		return frameCopyType;

	case LoRa::duplicate:
		break;

	case LoRa::resetDetected:
		ResetDetected();
		frameCopyArbitrator.AcceptAsFirstCopy(inferredSequenceNumber, frame.ThisServerReceiveTime_ms());
		break;

	default:
		;
	}

	frame.inferredSequenceNumber = inferredSequenceNumber;
	authenticationFailsSinceMostRecentAcceptedFrame = 0;
	return frameCopyType;
}


LoRa::FrameCopyType Mote::AuthenticateDataFrame(LoRa::ReceivedFrame const& frame, uint32& inferredSequenceNumberOutput) const	// returns true if the frame is acceptable
{
	uint16 newNumberL = frame.SequenceNumber();
	LoRa::FrameCopyType frameCopyType = frameCopyArbitrator.Test(newNumberL, frame.ThisServerReceiveTime_ms(), Global::networkServerToGatewayMaxDelay_ms);

	if (frameCopyType == LoRa::invalidDelay)
		return frameCopyType;

	uint32 previousAcceptedSequenceNumber = frameCopyArbitrator.LastUpMessageSequenceNumber();
	//The function must guess the top 16 bits of the sequence number

	if (frameCopyType == LoRa::duplicate || frameCopyType == LoRa::retransmission)
	{
		/*test only the previously accepted sequence number
		Assumes delay between messages is much greater than the difference in network delays from the gateway to this mote

		frameCopyType == LoRa::duplicate || frameCopyType == LoRa::retransmission  when the 16 bit sequence number number of the current frame equals that of the previous one.  This can
		occur when exactly 2^16-1 frames are missed, resulting in the rejection of 1 extra frame*/
		if (TestDataFrameAuthentication(frame, previousAcceptedSequenceNumber, true))
		{
			inferredSequenceNumberOutput = previousAcceptedSequenceNumber;
			return frameCopyType;
		}
	}
	else
	{
		if (!frameCopyArbitrator.FirstFrame())
		{
			uint16 previousSequenceNumberL = uint16(previousAcceptedSequenceNumber);
			uint16 previousSequenceNumberM = uint16(previousAcceptedSequenceNumber >> 16);
			uint16 newNumberM;

			//if the current 16 bit sequence number is greater than the previous rollover is unlikely
			if (newNumberL > previousSequenceNumberL)
				newNumberM = previousSequenceNumberM;
			else
				newNumberM = previousSequenceNumberM + 1;

			inferredSequenceNumberOutput = uint32(newNumberM << 16 | newNumberL);
			if (TestDataFrameAuthentication(frame, inferredSequenceNumberOutput, true))
				return frameCopyType; //normal return

			// failed to authenticate
			uint64 timeSinceLastMessage_ms = frame.ThisServerReceiveTime_ms() - frameCopyArbitrator.TimeOfPreviousAcceptedMessage_ms();

			if ((Global::timeToAssumeMoteLost_s > 0) && (timeSinceLastMessage_ms > (Global::timeToAssumeMoteLost_s * 1000)))
			{
				//it's been a very long time since a packet was accepted
				uint32 start = previousSequenceNumberM + (authenticationFailsSinceMostRecentAcceptedFrame / Global::moteMissSeqNoSearchRetries) + 1;
				//add one because authenticationFailsSinceMostRecentAcceptedFrame is zero on first pass

				if (start >= previousSequenceNumberM) // avoid testing lower values after the MSW has rolled over
				{
					uint32 end = start + Global::moteMissSeqNoSearchLimit;

					if (end > 0xFFFF)
						end = 0xFFFF;

					if (end < start)
						end = 0xFFFF;

					//test with most significant 16 bits of sequence number set to range above current value
					if (TestARangeOfSequenceNumbers(frame, start, end, inferredSequenceNumberOutput, false))
						return LoRa::first;
				}
			}

			if ((Global::timeToAssumeMoteReset_s > 0) && (timeSinceLastMessage_ms > (Global::timeToAssumeMoteReset_s * 1000)))
			{
				//it's been a very long time since a packet was accepted
				uint32 start = 0;
				uint32 end = start + Global::moteResetSeqNoSearchLimit;

				if (end > 0xFFFF)
					end = 0xFFFF;

				//test with most significant bits zero
				if (TestARangeOfSequenceNumbers(frame, start, end, inferredSequenceNumberOutput, false))
					return LoRa::resetDetected;
			}
		}
		else
		{
			//first packet test first maxNumberOfMissingSeqNumberMS MS values
			uint32 start = 0;
			uint32 end = start + Global::moteResetSeqNoSearchLimit;

			if (end > 0xFFFF)
				end = 0xFFFF;

			if (TestARangeOfSequenceNumbers(frame, start, end, inferredSequenceNumberOutput, false))
				return frameCopyType;
		}
	}

	return LoRa::failedAuthentication;
}


bool Mote::TestDataFrameAuthentication(LoRa::ReceivedFrame const& frame, uint32 sequenceNumber, bool checkForReplay) const
{
	if (checkForReplay && (sequenceNumber < frameCopyArbitrator.LastUpMessageSequenceNumber()))
	{
		if (Debug::Print(Debug::monitor))
		{
			std::stringstream logText;

			logText << "Mote " << AddressText(Id()) << " (Addr " << AddressText(NetworkAddress()) << ") failed seq number test " << std::hex << sequenceNumber << " last success " << frameCopyArbitrator.LastUpMessageSequenceNumber();
			Debug::Write(logText);
		}
		return false;
	}

	uint8 calculatedMic[LoRa::micBytes];

	LoRa::GenerateDataFrameIntegrityCode(NetworkSessionKey(), frame.Data(), frame.AuthenticatedLength() , NetworkAddress(), true, sequenceNumber, calculatedMic);

	bool result = (memcmp(calculatedMic, frame.Mic(), LoRa::micBytes) == 0) ? true : false;

	return result;
}


bool Mote::TestARangeOfSequenceNumbers(LoRa::ReceivedFrame const& frame, uint16 start, uint16 end, uint32& inferredSequenceNumberOutput, bool checkForReplay) const
{
	for (uint32 i = start; i <= end; i++)
	{
		inferredSequenceNumberOutput = (i << 16) | frame.SequenceNumber();
		if (TestDataFrameAuthentication(frame, inferredSequenceNumberOutput, checkForReplay))	// test with most significant bits zero
			return true;
	}

	return false;
}


void Mote::FrameSendOpportunity(uint8 transmissionWindow)
{
	if (!IsFrameReadyToTransmit())
		return;

	BestGateway const& bestGateway = transmitController.GetBestGateway();

	if (!bestGateway.Valid())
		return;

	TransmitFrame(bestGateway.Eui(), transmissionWindow);
}


bool Mote::DownstreamApplicationDataReceived(uint32 sequenceNumber, LoRa::FrameApplicationData const& payload, ValidValueUint16 const& token)	//to be sent to mote
{
	if (payload.Port() != LoRa::macCommandPort)
		return transmitController.SendApplicationData(sequenceNumber, payload, token);
	else
		return DownstreamMacCommandReceived(sequenceNumber, payload, token);
}


bool Mote::DownstreamMacCommandReceived(uint32 sequenceNumber, LoRa::FrameDataRecord const& input, ValidValueUint16 const& token)
{
	LoRa::FrameApplicationData macCmdPayload;

	macCmdPayload.SetLength(input.Length());
	macCmdPayload.Port(LoRa::macCommandPort);

	LoRa::EncryptPayload(NetworkSessionKey(), input.Data(), input.Length(), NetworkAddress(), false, sequenceNumber, macCmdPayload.DataNonConst());

	return transmitController.SendApplicationData(sequenceNumber, macCmdPayload, token);
}


void Mote::QueueLengthQueryReceived(TCP::Connection::IdType const& source, bool app)
{
	uint16 length = transmitController.QueueLength(app);

	SendQueueLengthToServer(source, app, length);
}


void MoteList::FrameReceived(LoRa::ReceivedFrame& frame)
{
	if (frame.IsDataFrame())
	{
		if (frame.Length() < frame.HeaderLength())
			throw LoRa::ReceivedFrame::MessageRejectException(frame, "Data frame too short ", frame.Length());

		if (frame.Version() != 0)
			throw LoRa::ReceivedFrame::MessageRejectException(frame, "Incorrect version ", frame.Version());

		if (frame.IsUpFrame())
			ReceivedDataFrame(frame);
		else
		{
			if (Debug::Print(Debug::verbose))
				Debug::Write("Self received \'down\' frame discarded");

			return;	//discard silently
		}
	}
	else if (frame.IsJoinFrame())
	{
		if (frame.IsJoinRequestFrame())
		{
			if (frame.Length() < LoRa::Frame::minJoinRequestBytes)
				throw LoRa::ReceivedFrame::MessageRejectException(frame, "Join request frame too short ", frame.Length());

			Global::joinController.ReceivedRequest(frame);
		}
		else
		{
			if (Debug::Print(Debug::verbose))
				Debug::Write("Self received \'join accept\' frame discarded");

			return;	//discard silently
		}
	}
	else
	{
		if (Debug::Print(Debug::verbose))
		{
			std::stringstream text;

			text << "Unexpected message type " << std::hex << int(frame.Type()) << " received";
			Debug::Write(text);
		}
	}
}


void MoteList::ReceivedDataFrame(LoRa::ReceivedFrame& frame)
{
	uint32 networkAddress = frame.Address();
	EuiType eui = Global::joinController.FindEui(networkAddress);

	//lint --e{429}  (Warning -- Custodial pointer 'mote' has not been freed or returned)
	if (eui == invalidEui)
		eui = EuiType(networkAddress);	//EUI may be an expansion of networkAddress

	Mote* mote = GetById(eui);  	//WARNING mote is always locked if not NULL

	bool autoCreated = false;
	if ((mote == 0) && Global::autoCreateMotes)
	{
		autoCreated = true;
		//Returns are safe here because mote has not been locked

		if (!Global::applicationList.Exists(nullEui))
		{
			if (Debug::Print(Debug::verbose))
			{
				std::stringstream text;

				text << "Mote (address " << AddressText(frame.Address()) << ") received frame but has not joined and null application is unknown";
				Debug::Write(text);
			}
			return;
		}
		mote = new Mote(EuiType(networkAddress), nullEui, networkAddress, Global::defaultAuthenticationKey, invalidValueUint32, invalidValueUint32);

		mote->Lock(); //to match the mote that has not just been created
		eui = mote->Id();
	}

	if (!mote)
		throw LoRa::ReceivedFrame::MessageRejectException(frame, std::string("Message received from unknown network address ") + AddressText(networkAddress));

	//mote is always locked here
	if (Debug::Print(Debug::verbose))
	{
		std::stringstream logText;
		
		logText << (frame.IsDataConfirmedFrame() ? "Confirmed " : "Unconfirmed ") <<
			"frame received from Mote " << AddressText(mote->Id()) << " (Net Addr:" << AddressText(mote->NetworkAddress()) << ")  "
			"GW " << AddressText(frame.GatewayEui()) << "  ("<< frame.GatewayAddress().Text() <<")  " << 
			"Seq# " << std::hex << frame.SequenceNumber();

		Debug::Write(logText);
	}
	mote->ReceivedDataFrame(frame);
	mote->Unlock();

	if (autoCreated)
	{
		//no need for mote to be locked - because autocreated motes aren't in the list so can't be found
		if (mote->AtLeastOneMessageReceived())
			Add(mote);	//add to list
		else
			delete mote;
	}
}


void MoteList::CreateMote(EuiType myEui, EuiType const& myApplication, uint32 networkAddress, LoRa::CypherKey const& myAuthenticationKey)
{
	Mote* mote = new Mote(myEui, myApplication, networkAddress, myAuthenticationKey, invalidValueUint32, invalidValueUint32);

	Add(mote);

	if (Debug::Print(Debug::monitor))
	{
		std::stringstream logText;
		
		logText << "Mote " << AddressText(myEui) << " (" << AddressText(networkAddress) << ") created for application " << AddressText(myApplication);

		Debug::Write(logText);
	}
}

EuiType MoteList:: GetAppEui(EuiType eui) const
{
	Mote* mote = GetById(eui);  	//WARNING mote is always locked if not NULL

	if (!mote)
		return invalidEui;

	EuiType result = mote->ApplicationEui();
	mote->Unlock();
	return result;
}


bool MoteList::DownstreamApplicationDataReceived(EuiType moteEui, uint32 sequenceNumber, LoRa::FrameApplicationData const& payload, ValidValueUint16 const& token) const	//to be sent to mote
{
	Mote* mote = GetById(moteEui);  	//WARNING mote is always locked if not NULL

	if (!mote)
		return false;

	bool result = mote->DownstreamApplicationDataReceived(sequenceNumber, payload, token);
	mote->Unlock();
	return result;
}


void Mote::ResetDetected()
{
	if (!IsProvisionedMote())
		return;

	InformUpstreamServerOfMoteReset();
	frameCopyArbitrator.Reset();
	transmitController.Reset();
}


void Mote::ForwardHeaderOptions(LoRa::Frame const& frame)
{
	LoRa::OptionRecord headerOptions;

	headerOptions.SetData(frame.OptionStart(), frame.OptionLength());

	ForwardHeaderOptions(headerOptions);
}


void Mote::ForwardHeaderOptions(LoRa::OptionRecord const& headerOptions)
{
	JSON::String jsonString;
	jsonString.Open();
	jsonString.AddMacCommand(Id(), invalidValueUint16, headerOptions, false);
	jsonString.Close();	//top

	Global::applicationList.Send(ApplicationEui(), Service::macCommandServer, jsonString);
}


void Mote::SendQueueLengthToServer(TCP::Connection::IdType const& server, bool app, uint16 length)
{
	JSON::String jsonString;

	jsonString.Open();
	jsonString.AddQueueLength(Id(), app, length, false);
	jsonString.Close();	//top

	JSON::Send(server, jsonString);
}


void Mote::InformUpstreamServerOfMoteReset()
{
	JSON::String jsonString;

	jsonString.Open();
	jsonString.AddResetDetected(false);
	jsonString.Close();
	Global::applicationList.Send(ApplicationEui(), Service::moteTxServer || Service::userDataServer, jsonString);
}

void Mote::InformUpstreamServerOfTransmissionToMote(bool app, uint16 token)
{
	JSON::String jsonString;

	jsonString.Open();
	jsonString.AddMessageSent(Id(), app, token, false);
	jsonString.Close();
	Global::applicationList.Send(ApplicationEui(), Service::userDataServer, jsonString);
}


void Mote::InformUpstreamServerOfAcknowledgementFromMote(bool app, uint16 token)
{
	JSON::String jsonString;

	jsonString.Open();
	jsonString.OpenStructuredObject("mote", false);
	jsonString.AddEui("eui",Id(), false);
	jsonString.AddUnsignedValue("ackrx",token);

	jsonString.Close();	//app
	jsonString.Close();	//top
	Global::applicationList.Send(ApplicationEui(), Service::userDataServer, jsonString);
}


void Mote::FrameSequenceNumberGrantRequestReceived(MessageAddress const& source)
{
	uint32 sequenceNumber = transmitController.GetNextDownstreamSequenceNumber();

	JSON::String jsonString;
	
	jsonString.Open();
	jsonString.AddSequenceNumberGrant(Id(), sequenceNumber, false);
	jsonString.Close();

	JSON::Send(source, jsonString);
}


bool Mote::UpdateNVStoreSequenceNumber()
{
	bool updateTx = transmitController.StoreSeqNoInNVStore();	//ensure both functions are called because calling them resets the values;
	bool updateRx = frameCopyArbitrator.StoreSeqNoInNVStore();

	if (!updateTx && !updateRx)
		return true;

	return Global::networkDatabase.UpdateMoteSequenceNumbers(Id(), transmitController.GetCurrentDownstreamSequenceNumber(), frameCopyArbitrator.LastUpMessageSequenceNumber());

}

