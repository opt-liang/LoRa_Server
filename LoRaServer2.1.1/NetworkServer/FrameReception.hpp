/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef FRAME_RECEPTION_HPP
#define FRAME_RECEPTION_HPP

#include "General.h"
#include "Eui.hpp"
#include "TransmissionRecord.hpp"
#include "LoRa.hpp"
#include "LoRaReceiveFrame.hpp"
#include "MessageAddress.hpp"
#include "Mutex.hpp"
#include "JsonString.hpp"
#include "UdpSocket.hpp"

#include <list>


class FrameReceptionRecord
{
private:
	//Id
	const EuiType						moteEui;

	//Data
	LoRa::FrameApplicationDataSQ		payload;
	MoteTransmitRecord					transmit;
	GatewayReceiveList					gatewayReceiveList;

	bool								responseRequested;

public:
	FrameReceptionRecord(EuiType myMoteEui, 
		uint32 mySequenceNumber, 
		uint8 const myPayload[],
		uint16 myPayloadLength,
		uint8 myMotePort,
		uint32 myFrequency_Hz,
		LoRa::DataRate const& myDataRate,
		LoRa::CodeRate const& myCodeRate,
		bool myAdrEnabled, bool myResponseRequested)
	: moteEui(myMoteEui),
		payload(myPayload, myPayloadLength, myMotePort, mySequenceNumber),
		transmit(myFrequency_Hz, myDataRate, myCodeRate, myAdrEnabled),
		responseRequested(myResponseRequested)
	{}

	FrameReceptionRecord(EuiType myMoteEui, LoRa::FrameApplicationDataSQ const& myPayload, MoteTransmitRecord const& myTransmit, bool myResponseRequested)
	: moteEui(myMoteEui),
		payload(myPayload),
		transmit(myTransmit),
		responseRequested(myResponseRequested)
	{}


	FrameReceptionRecord(FrameReceptionRecord const& other);

	virtual ~FrameReceptionRecord() {}

	bool AddGateway(EuiType myEui, //returns true if transmission record must be updated (previous frame was a ghost)
		TimeRecord const& myReceiveTime, 
		ValidValueUint32 myReceiveTimestamp_us,	//NOT related to any recognised timescale
		ValidValueUint16 myChannel,
		ValidValueUint16 myRxRfChain,
		ValidValueSint16 mySignalToNoiseRatio_cB,
		ValidValueSint16 mySignalStrength_cBm)
	{return gatewayReceiveList.Add(myEui, myReceiveTime, myReceiveTimestamp_us, myChannel, myRxRfChain, mySignalToNoiseRatio_cB, mySignalStrength_cBm);}

	bool AddGateway(GatewayReceiveRecord const& myGateway) {return gatewayReceiveList.Add(myGateway);}

	void UpdateFrequency(uint32 frequency_Hz) {transmit.frequency_Hz = frequency_Hz;}

	uint16 NumberOfGateways() const {return gatewayReceiveList.Size();}

	EuiType Eui() const {return moteEui;}
	uint32 SequenceNumber() const {return payload.SequenceNumber();}

	LoRa::FrameApplicationDataSQ const& Payload() const {return payload;}
	GatewayReceiveList const& GatewayList() const {return gatewayReceiveList;}
	bool ResponseRequested() const {return responseRequested;}

	void Transmit();
};


class FrameReceptionListElement : public FrameReceptionRecord
{
private:
	const uint64						timeToTransmit_ms;
public:
	FrameReceptionListElement(
		uint64 myTimeToTransmit_ms,

		EuiType myMoteEui, 
		uint32 mySequenceNumber, 
		uint8 const myPayload[],
		uint16 myPayloadLength,
		uint8 myMotePort,
		uint32 myFrequency_Hz,
		LoRa::DataRate const& myDataRate,
		LoRa::CodeRate const& myCodeRate,
		bool myAdrEnabled, bool myResponseRequested)
		:FrameReceptionRecord(myMoteEui, 
			mySequenceNumber, myPayload, myPayloadLength, myMotePort,
			myFrequency_Hz, myDataRate,
			myCodeRate, myAdrEnabled, myResponseRequested),
			timeToTransmit_ms(myTimeToTransmit_ms)
	{
	}

	FrameReceptionListElement(
		uint64 myTimeToTransmit_ms,
		EuiType myMoteEui,
		LoRa::FrameApplicationDataSQ const& myPayload, 
		MoteTransmitRecord const& myTransmit, bool myResponseRequested)

	: FrameReceptionRecord(myMoteEui, myPayload, myTransmit, myResponseRequested),
			timeToTransmit_ms(myTimeToTransmit_ms)
	{
	}

	uint64 TimeToTransmit_ms() const {return timeToTransmit_ms;}

	void Transmit() {FrameReceptionRecord::Transmit();}

private:
};

class FrameReceptionList
{
private:
	std::list<FrameReceptionListElement*>			list;
	Mutex											mutex;

public:
	//lint -e{1509} (Warning -- base class destructor for class 'list' is not virtual)
	virtual ~FrameReceptionList()
	{
		MutexHolder holder(mutex);
		while (!list.empty())
		{
			delete list.front();

			list.pop_front();
		}
	}

	void Store(LoRa::FrameCopyType frameCopyType, 
		EuiType moteEui, 
		LoRa::ReceivedFrame const& frame, 
		uint64 forwardTime_ms, 
		TimeRecord const& frameReceiveTime)
	{
		//lint --e{429} (Warning -- Custodial pointer 'record' (line 129) has not been freed or returned)
		MutexHolder holder(mutex);

		FrameReceptionListElement* record;
		if (frameCopyType == LoRa::duplicate)
		{
			record = Find(moteEui, frame.inferredSequenceNumber);

			if (!record)
				return;	//duplicate but no original available
		}
		else
		{
			//frameCopyType is one of first, retransmission or resetDetected
			record = new FrameReceptionListElement(forwardTime_ms, moteEui, frame.inferredSequenceNumber, frame.Payload(), frame.PayloadLength(), 
				frame.Port(), frame.frequency_Hz, frame.dataRate, frame.codeRate, frame.AdrEnabled(), frame.RequestingResponse());
			list.push_back(record);
		}

		bool updateTransmitFrequency = record->AddGateway(frame.GatewayEui(), frameReceiveTime, frame.gatewayReceiveTimestamp_us, frame.channel, frame.rfChain, frame.signalToNoiseRatio_cB, frame.signalStrength_cBm);

		if (updateTransmitFrequency)
			record->UpdateFrequency(frame.frequency_Hz);
	}

	FrameReceptionListElement* GetNextTxRecord(uint64 maxTxTime)	//record must be deleted by calling function
	{
		MutexHolder holder(mutex);
		if (list.empty())
			return 0;

		if (list.front()->TimeToTransmit_ms() > maxTxTime)
			return 0;

		FrameReceptionListElement* result = list.front();
		list.pop_front();

		return result;
	}

	void Tick(uint64 now)
	{
		//doesn't hold mutex to avoid holding it for too long
		FrameReceptionListElement* element;

		//lint -e{720} (Info -- Boolean test of assignment)
		while (element = GetNextTxRecord(now))
		{
			element->Transmit();
			delete element;
		}
	}

private:

	FrameReceptionListElement* Find(EuiType moteEui, uint32 sequenceNumber)
	{
		if (list.empty())
			return 0;

		std::list<FrameReceptionListElement*>::reverse_iterator it = list.rbegin();

		for (;it != list.rend(); ++it)
		{
			if ((*it)->Eui() == moteEui && (*it)->SequenceNumber() == sequenceNumber)
				return *it;
		}
		return 0;
	}
};



#endif

