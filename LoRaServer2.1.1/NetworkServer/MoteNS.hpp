/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef MOTE_NS_HPP
#define MOTE_NS_HPP

#include "General.h"
#include "BinarySearchVectorNV.hpp"
#include "LoRa.hpp"
#include "ExceptionClass.hpp"
#include "Utilities.hpp"
#include "TransmitQueue.hpp"
#include "MoteTransmitController.hpp"
#include "Application.hpp"

#include <memory.h>
#include <time.h>
#include <string>

class MoteList;

class Mote : public BinarySearchVector::ElementTemplate<Mote, EuiType>
{
	friend class MoteList;
	friend class BinarySearchVectorNV::List<class Mote, EuiType>;

public:
	static const uint64							defaultTimeToAssumeReset_s = 48 * 60 * 60;	//reset of mote
	static const uint64							defaultTimeToAssumeContactLost_s = 1 * 60 * 60;
	static const uint16							defaultMissingSeqNoSearchLimit = 4;
	static const uint16							defaultResetSeqNoSearchLimit = 4;
	static const uint16							defaultMissingSeqNoSearchRetries = 3;	//defined in .cpp file because of problem with gcc compilation
	static const unsigned						maxTimeStringLength = 60;
	static const uint64							invalidTimeOfLastMessage = ~uint64(0);	//all ones
	static const EuiType						defaultApplicationEui = 0;
	static const uint8							defaultReceiveWindow = 0;	//1st window


private:
	class FrameCopyArbitrator
	{
	private:
		ValidValueUint32			sequenceNumber;		//the 32 bit sequence number of the current frame (valid when at least one frame has been accepted since creation
		uint64						serverRxTime_ms;	//also the latest time the first copy of the current frame could have been transmitted
									//	in milliseconds.  Rollover happens in apprx 5.8E14 years.
		bool						storeSeqNoInNVStore;	//Upstream number

	public:
		FrameCopyArbitrator(ValidValueUint32 const& mySequenceNumber)
			: sequenceNumber(mySequenceNumber), serverRxTime_ms(0), storeSeqNoInNVStore(false)
		{}

		LoRa::FrameCopyType Test(uint16 newSequenceNumber, uint64 receiveTime_ms, uint64 maxGatewayToNodeTransmissionTime_ms) const	//returns only first, duplicate, invalidDelay or retransmission
		{
			if (!sequenceNumber.Valid())
				return LoRa::first;

			else if (uint16(sequenceNumber) != newSequenceNumber)
				return LoRa::first;

			//repeat number - use receive time to distinguish
			else if (serverRxTime_ms + maxGatewayToNodeTransmissionTime_ms >= receiveTime_ms)
				return LoRa::duplicate;	//close enough together to same another copy for the same mote transmission
			else if (serverRxTime_ms + (LoRa::minMoteRetransmissionDelay_ms - maxGatewayToNodeTransmissionTime_ms) >= receiveTime_ms)
				return LoRa::invalidDelay;	//too for away to be accepted as a dupliate reception but too close to be a retransmission
			else
				return LoRa::retransmission;	//too far apart to be from a single transmission - must be from a retransmission
		}

		void AcceptAsFirstCopy(uint32 newSequenceNumber, uint64 receiveTime_ms)	//called when frame is accepted by mote
		{
			storeSeqNoInNVStore |= uint16(sequenceNumber) > uint16(newSequenceNumber) ? true : false;	//store when LSBs rollover
			sequenceNumber = newSequenceNumber;
			serverRxTime_ms = receiveTime_ms;
		}

		void Reset()
		{
			sequenceNumber.SetInvalid();
			serverRxTime_ms = 0;
		}

		bool FirstFrame() const {return !sequenceNumber.Valid();}
		uint64 TimeOfPreviousAcceptedMessage_ms() const {return serverRxTime_ms;}
		uint32 LastUpMessageSequenceNumber() const {return sequenceNumber;}

		bool StoreSeqNoInNVStore()	//returns true if the sequence number has rolled over
		{
			bool result = storeSeqNoInNVStore;
			storeSeqNoInNVStore = false;
			return result;
		}
	};

	EuiType										appEui;
	uint32										networkAddress;
	LoRa::CypherKey								networkSessionKey;
	FrameCopyArbitrator							frameCopyArbitrator;
	sint16										battery; // 255 fully charged, -ve is invalid
	sint16										rxSnrMargin_cB;	//mote receive SNR margin in centiBells
	MoteTransmitController						transmitController;
	uint16										authenticationFailsSinceMostRecentAcceptedFrame;	// a count that will eventually rollover

	//motes are only created by MoteList
	Mote(EuiType myEui, EuiType myAppEui, uint32 myNetworkAddress, LoRa::CypherKey const& myNetworkSessionKey, 
			ValidValueUint32 const& myLastDownMessageSequenceNumber, ValidValueUint32 const& myLastUpMessageSequenceNumber)
		: BinarySearchVector::ElementTemplate<Mote, EuiType>(myEui),
		appEui(myAppEui),
		networkAddress(myNetworkAddress),
		networkSessionKey(myNetworkSessionKey),
		frameCopyArbitrator(myLastUpMessageSequenceNumber),
		battery(SHRT_MIN),
		rxSnrMargin_cB(SHRT_MIN),
		transmitController(*this, myLastDownMessageSequenceNumber),
		authenticationFailsSinceMostRecentAcceptedFrame(0)
	{}

	~Mote() {}
public:

	void NetworkAddress(uint32 nw) {networkAddress = nw;}
	uint32 NetworkAddress() const {return networkAddress;}

	LoRa::CypherKey const& NetworkSessionKey() const {return networkSessionKey;}

	void ReceivedDataFrame(LoRa::ReceivedFrame& frame);
	bool IsProvisionedMote() const {return appEui == defaultApplicationEui;} //return true if mote was not created via Join Request/Response

	void Status(uint8 myBattery, uint8 myRxSnrMargin_dB)
	{
		battery = myBattery;
		rxSnrMargin_cB = myRxSnrMargin_dB * 10;
	}

	void SetBestGateway(BestGateway const& newBestGateway) {transmitController.SetBestGateway(newBestGateway);}
	bool IsFrameReadyToTransmit() const {return transmitController.IsFrameReadyToTransmit();}

	EuiType ApplicationEui() const {return appEui;}

	EuiType const& ClientApplicationEui() const {return appEui;}
	void SendFrameToMote(LoRa::Frame const& frame, bool isJoinAcceptFrame, uint8 transmissions = 0) {transmitController.SendFrame(frame, isJoinAcceptFrame, transmissions);} //only used for Join accepts - other data is sent via DownstreamApplicationDataReceived, zero transmissions represents default
	void FrameSendOpportunity(uint8 transmissionWindow);
		/* informs mote of the opportunity to send a frame.  
		transmissionWindow is window to be used - this passed as a paramter (even though its source is TransmitWindow()) because the value may have changed 
		between time that mote transmitted the previous frame and the time the mote will be ready to accept the transmitted one */

	bool DownstreamApplicationDataReceived(uint32 sequenceNumber, LoRa::FrameApplicationData const& payload, ValidValueUint16 const& token);	//to be sent to mote
	void SendMacCommand(LoRa::OptionRecord const& option, ValidValueUint16 const& token) {transmitController.SendMacCommand(option, token);}

	bool MoteTransmitQueueContainsAppData() const {return transmitController.AppDataWaitingToBeTransmitted();}

	void ResetDetected(); // mote reset detected
	void InformUpstreamServerOfTransmissionToMote(bool app, uint16 token);	//if false, transmission was a MAC command
	void InformUpstreamServerOfAcknowledgementFromMote(bool app, uint16 token);

	void FrameSequenceNumberGrantRequestReceived(MessageAddress const& source);
	void QueueLengthQueryReceived(TCP::Connection::IdType const& source, bool app);

private:
	bool AtLeastOneMessageReceived() const {return !frameCopyArbitrator.FirstFrame();}

	//All data frames contain LoRa::binaryMessageBytes
	LoRa::FrameCopyType AuthenticateAndAcceptDataFrame(LoRa::ReceivedFrame& frame);
		// returns true if the frame is acceptable.
		//frame.inferredSequenceNumber is updated to match the inferred (32 bit) value calculated from the 16 bit value in the frame

	LoRa::FrameCopyType AuthenticateDataFrame(LoRa::ReceivedFrame const& frame, uint32& inferredSequenceNumberOutput) const;	// returns true and updates 'inferredSequenceNumberOutput' if the frame is acceptable.  Throws MessageRejectException if not
	bool TestDataFrameAuthentication(LoRa::ReceivedFrame const& frame, uint32 sequenceNumber, bool checkForReplay) const;

	bool TestARangeOfSequenceNumbers(LoRa::ReceivedFrame const& frame, uint16 start, uint16 end, uint32& inferredSequenceNumberOutput, bool checkForReplay) const; 
	//'start' and 'end' are the most significant 16 bits of the sequence number; the least sigificant 16 bits are carried in 'frame'
	bool TestARangeOfSequenceNumbers(LoRa::ReceivedFrame const& frame, uint32 start, uint32 end, uint32& inferredSequenceNumberOutput, bool checkForReplay) const
		{return TestARangeOfSequenceNumbers(frame, static_cast<uint16>(start), static_cast<uint16>(end), inferredSequenceNumberOutput, checkForReplay);} //MSW of start and end is ignored
		//from recordData.lastTrialSequenceNumberM to recordData.lastTrialSequenceNumberM + maxTestsPerReceivedMessage
		//returns true on success and updates inferredSequenceNumber

	void SetValid();
	bool DownstreamMacCommandReceived(uint32 sequenceNumber, LoRa::FrameDataRecord const& payload, ValidValueUint16 const& token);

	void TransmitFrame(EuiType gatewayEui, uint8 transmissionWindow) {transmitController.TransmitFrame(gatewayEui, transmissionWindow);}
	void ForwardHeaderOptions(LoRa::Frame const& frame);
	void ForwardHeaderOptions(LoRa::OptionRecord const& macCommandRecord);

	void SendQueueLengthToServer(TCP::Connection::IdType const& source, bool app, uint16 length);
	void InformUpstreamServerOfMoteReset();

	void SendStringToServer(std::string const& text) const;

	bool UpdateNVStoreSequenceNumber();

public:
	//redefined virtual functions
	void CreateNVRecord() const;
};


class MoteList : public BinarySearchVectorNV::List<Mote, EuiType>
{
public:
	MoteList(bool initialise)
		: BinarySearchVectorNV::List<Mote, EuiType>(initialise)
	{}
	//if initialise is false Initialise() must be called before any action is performed
	//if initialise is true may throw 'Exception' (ExceptionClass.hpp)

	void FrameReceived(LoRa::ReceivedFrame& frame);
	void ReceivedDataFrame(LoRa::ReceivedFrame& frame);

	void CreateMote(EuiType myEui, EuiType const& myApplication, uint32 networkAddress, LoRa::CypherKey const& myAuthenticationKey);

	//lint --e{1774} (Info -- Could use dynamic_cast to downcast polymorphic type 'BinarySearchVector::ElementTemplate<Mote,unsigned long long>')
	Mote* GetById(EuiType eui) const {return static_cast<Mote*>(BinarySearchVectorNV::List<Mote, EuiType>::GetById(eui));} 	//Mote is locked
	Mote* GetByIndex(uint32 index) const {return static_cast<Mote*>(BinarySearchVectorNV::List<Mote, EuiType>::GetByIndex(index));} 	//Mote is locked
	bool DeleteById(EuiType eui);	//hides BinarySearchVectorNV::List<Mote, EuiType> function

	EuiType GetAppEui(EuiType eui) const;

	bool DownstreamApplicationDataReceived(EuiType moteEui,uint32 sequenceNumber, LoRa::FrameApplicationData const& payload, ValidValueUint16 const& token) const;	//to be sent to mote
private:
	void Add(Mote* mote) {BinarySearchVectorNV::List<Mote, EuiType>::Add(mote);}

};

#endif

