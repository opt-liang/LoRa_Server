/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef SPREADING_FACTOR_CONTROLLER_HPP
#define SPREADING_FACTOR_CONTROLLER_HPP

#include "WaitingFrame.hpp"
#include "OptionController.hpp"
#include "LoRa.hpp"
#include "LoRaReceiveFrame.hpp"
#include "BestGateway.hpp"
#include "JsonString.hpp"
#include "ValueWithValidity.hpp"

#include <list>

class Mote;

class MoteTransmitController
{
private:
	static const uint32		downMessageSequenceNumberMask = 0x03FF;

	class LinkCheckController : public OptionController
	{
	private:
		uint32					sequenceNumber;
		sint16					margin_cB;
		sint16					gateways;

	public:
		LinkCheckController()
			: sequenceNumber(~0), margin_cB(SHRT_MIN), gateways(0)
		{
			command.SetLength(3);
			command[0] = uint8(LoRa::linkCheck);
			//command[1] holds the margin
			//command[2] holds the gateway count
		}

		void CommandReceived(uint32 mySequenceNumber, sint16 myMargin_cB)
		{
			if (sequenceNumber == mySequenceNumber)
			{
				if (margin_cB < myMargin_cB)
					margin_cB = myMargin_cB;

				gateways++;
			}
			else
			{
				Active(true);
				sequenceNumber = mySequenceNumber;
				margin_cB = myMargin_cB;
				gateways = 1;
			}
		}

	private:
		sint8 Margin_dB() const {return sint8((margin_cB > 0) ?  (margin_cB / 10) : 0);}
		uint8 Gateways() const {return (gateways < 0xFF) ? uint8(gateways) : 0xFF;}

		void UpdateCommand()	//redefined virtual function
		{
			command[1] = uint8(Margin_dB());
			command[2] = Gateways();
		}
	};

	class DeviceStatusRequester : public OptionController
	{
	public:
		DeviceStatusRequester()
		{
			command.SetLength(1);
			command[0] = uint8(LoRa::devStatus);
		}
	private:
		void UpdateCommand() {}	//redefined virtual function
	};

	class ApplicationData				//wating for transmission
	{
	private:
		LoRa::FrameApplicationDataSQ	payload;
		ValidValueUint16				token;

	public:
		void Set(uint32 sequenceNumber, LoRa::FrameApplicationData const& other) {payload.SequenceNumber(sequenceNumber); payload.Set(other); token.SetInvalid();}
		void Set(uint32 sequenceNumber, LoRa::FrameApplicationData const& other, ValidValueUint16 const& otherToken) {payload.SequenceNumber(sequenceNumber); payload.Set(other); token = otherToken;}
		void Set(uint32 sequenceNumber, LoRa::FrameApplicationData const& other, uint16 otherToken) {payload.SequenceNumber(sequenceNumber); payload.Set(other); token = otherToken;}

		bool Valid() const {return payload.Valid();}
		void Clear() {payload.Clear(); token.SetInvalid();}

		uint8 Port() const {return payload.Port();}
		uint16 Length() const {return payload.Length();}
		bool AcknowledgeRequested() const {return payload.AcknowledgeRequested();}
		uint8 const* Data() const {return payload.Data();}
		ValidValueUint32 const& SequenceNumber() const {return payload.SequenceNumber();}
		void SequenceNumber(uint32 s) {payload.SequenceNumber(s);}

		ValidValueUint16 Token() const {return token;}
	};

	struct MacCommandRecord
	{
		LoRa::OptionRecord			option;
		ValidValueUint16			token;
	};

	class MacCommandStore
	{
	private:
		std::list<MacCommandRecord>	list;
		uint16						bytes;
	
	public:
		MacCommandStore() : bytes(0)
		{}

		void Add(LoRa::OptionRecord const& newOption, ValidValueUint16 const& newToken)
		{
			MacCommandRecord command;
			command.option = newOption;
			command.token = newToken;

			std::list<MacCommandRecord>::iterator it = list.begin();

			while (it != list.end() && it->option[0] <= newOption[0])	//insert in increasing order of command identified (CID)
				it++;

			if (it == list.end())
				list.push_back(command);
			else
				list.insert(it,command);

			bytes += newOption.Length();
		}

		uint16 Length() const {return bytes;}
		bool Active() const {return Length() > 0;}
		MacCommandRecord const& Next() const {return list.front();}
		MacCommandRecord GetNext()
		{
			MacCommandRecord result = list.front(); 
			bytes -= result.option.Length();
			list.pop_front();
			return result;
		}
	};


	Mote&						mote;
	WaitingFrame				waitingFrame;	// length is zero when the frame has been transmitted and acknowledgment (if requred has been received)
	bool						moteRequestingResponse;
	bool						moteRequestingAcknowledgement;	//sent a dataConfirmedFrame
	LinkCheckController			linkCheckController;
	DeviceStatusRequester		deviceStatusRequester;
	BestGateway					bestGateway;
	ApplicationData				waitingApplicationData;
	MacCommandStore				waitingMacCommands;
	uint32						nextDownstreamSequenceNumber;
	bool						storeSeqNoInNVStore;	//Downstream number - NV store is periodically updated

public:
	MoteTransmitController(Mote& myMote, ValidValueUint32 const& myNextTxSequenceNumber)
		:mote(myMote), waitingFrame(myMote),
		moteRequestingResponse(false), 
		moteRequestingAcknowledgement(false),
		nextDownstreamSequenceNumber(myNextTxSequenceNumber.Valid() ? 	((myNextTxSequenceNumber+1) | downMessageSequenceNumberMask) : 0), //if known move it forward, otherwise start from zero
		storeSeqNoInNVStore(false)
	{}

	void ReceivedFrame(LoRa::FrameCopyType frameCopyType, LoRa::ReceivedFrame const& frame);
	void ReceiveOption(LoRa::FrameCopyType frameCopyType, LoRa::ReceivedFrame const& frame, uint8 const options[], uint16 length);

	bool RequestingResponse() const {return moteRequestingResponse;}

	bool IsFrameReadyToTransmit() const;
	uint8 QueueLength(bool app) const;
	void TransmitFrame(EuiType gatewayEui, uint8 transmissionWindow);	// called to cause immediate frame transmission

	//Input functions
	bool SendApplicationData(uint32 sequenceNumber, LoRa::FrameApplicationData const& data, ValidValueUint16 const& token);
	void SendFrame(LoRa::Frame const& frame, bool isJoinAcceptFrame, uint8 transmissions = 0) {waitingFrame.Set(frame, isJoinAcceptFrame, transmissions);}	//zero represents default number of transmissions
	void SendMacCommand(LoRa::OptionRecord const& option, ValidValueUint16 const& token) {waitingMacCommands.Add(option, token);}
	void SetBestGateway(BestGateway const& newBestGateway) {bestGateway = newBestGateway;}

	BestGateway const& GetBestGateway() const {return bestGateway;}

	bool AppDataWaitingToBeTransmitted() const {return waitingApplicationData.Valid();}

	uint32 GetNextDownstreamSequenceNumber()
	{
		uint32 result = nextDownstreamSequenceNumber++;

		storeSeqNoInNVStore |= ((result ^ nextDownstreamSequenceNumber) & ~downMessageSequenceNumberMask) ? true : false;
		return result;
	}

	uint32 GetCurrentDownstreamSequenceNumber() const {return nextDownstreamSequenceNumber;}	//Does NOT increment number
	void Reset() {nextDownstreamSequenceNumber = 0;}

	bool StoreSeqNoInNVStore()	//returns true if the sequence number has rolled over
	{
		bool result = storeSeqNoInNVStore;
		storeSeqNoInNVStore = false;
		return result;
	}

private:
	void GenerateDataFrame();	//creates a LoRa frame and writes it to waitingFrame
};

#endif

