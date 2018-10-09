/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef MOTENC_HPP
#define MOTENC_HPP

#include "General.h"
#include "BinarySearchVectorNV.hpp"
#include "LoRa.hpp"
#include "TransmissionRecord.hpp"
#include "ExceptionClass.hpp"
#include "Utilities.hpp"
#include "Eui.hpp"
#include "MoteAlgorithmContainer.hpp"
#include "WordStore.hpp"

class Mote : public BinarySearchVector::ElementTemplate<Mote, EuiType>
{
public:
	static const EuiType						defaultApplicationEui = 0;


private:
	EuiType								appEui;
	MoteAlgorithmContainer				algorithmContainer;
	bool								adrEnabled;			//value received from mote
	EuiType								bestGatewayEui;		// best gateway of the most recently received message or invalidEui if none received
	MoteTransmitRecord					mostRecentTransmission;

public:
	Mote(EuiType moteEui, EuiType appEui);

	EuiType ApplicationEui() const {return appEui;}

	void ApplicationDataReceived(bool up, ValidValueUint32 const& sequenceNumber, ValidValueUint16 token,
		LoRa::FrameApplicationData const& payload, 
		MoteTransmitRecord const& transmitRecord, GatewayReceiveList const& gatewayReceiveList);

	void JoinedNetwork(uint16 deviceNonce);
	void AcknowledgeReceived(bool app);
	void QueueLengthQueryReceived(bool app);
	void QueueLengthReceived(bool app, uint16 length);
	void ResetDetected() {Reset();}
	void Reset() {}

	bool AdrEnabled() const  {return adrEnabled;}
	LoRa::ValidRegion GetRegion() const;
	void ReminderReceived(uint16 algorithmId) {algorithmContainer.ReminderReceived(algorithmId);}

	bool ReceiveServiceCommand(WordStore& commandWordStore) {return algorithmContainer.CommandReceived(commandWordStore);}
public:
	//redefined virtual functions
	void CreateNVRecord() const;
};


class MoteList : public BinarySearchVectorNV::List<Mote, EuiType>
{
public:
	MoteList(bool initialise)
		:BinarySearchVectorNV::List<Mote, EuiType>(initialise)
	{}
	//if initialise is false Initialise() must be called before any action is performed
	//if initialise is true may throw 'Exception' (ExceptionClass.hpp)

	void CreateMote(EuiType myEui, EuiType appEui);

	void ApplicationDataReceived(bool up, EuiType moteEui, ValidValueUint32 const& sequenceNumber, ValidValueUint16 const& token,
		LoRa::FrameApplicationData const& payload, MoteTransmitRecord const& transmitRecord, GatewayReceiveList const& gatewayReceiveList);

	Mote* GetById(EuiType eui) const {return static_cast<Mote*>(BinarySearchVectorNV::List<Mote, EuiType>::GetById(eui));}
	Mote* GetByIndex(uint32 index) const {return static_cast<Mote*>(BinarySearchVectorNV::List<Mote, EuiType>::GetByIndex(index));}
	bool Delete(EuiType eui) {return BinarySearchVectorNV::List<Mote, EuiType>::DeleteById(eui);}

	bool QueueLengthQueryReceived(EuiType eui);
	bool QueueLengthReceived(TCP::Connection::IdType& source, EuiType eui, uint16 length);
	bool MoteResetDetected(EuiType eui);	//returns true if mote found

	void ReminderReceived(EuiType eui, uint16 algorithmId);

	//Service commands
	bool ReceiveMoteServiceCommand(EuiType eui, WordStore& commandWordStore);	//returns false on any failure

private:
	void Add(Mote* mote) {BinarySearchVectorNV::List<Mote, EuiType>::Add(mote);}
};

#endif

