/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef MOTE_NS_HPP
#define MOTE_NS_HPP

#include "General.h"
#include "BinarySearchVectorNV.hpp"
#include "LoRa.hpp"
#include "TransmissionRecord.hpp"
#include "ExceptionClass.hpp"
#include "Utilities.hpp"

class Mote : public BinarySearchVector::ElementTemplate<Mote, EuiType>
{
private:
	class DownstreamDataController
	{
	private:
		Mote&						mote;
		ValidValueUint16			token;
		LoRa::FrameApplicationData	receivedData;

	public:
		DownstreamDataController(Mote& myMote) 
			: mote(myMote)
		{}

		void Clear()
		{
			token.SetInvalid();
			receivedData.Clear();
		}

		void DownstreamApplicationDataReceived(LoRa::FrameApplicationData const& payload, ValidValueUint16 const& newToken)
		{
			if (!payload.Valid())
			{
				Clear();
				return;
			}

			token = newToken;
			receivedData = payload;

			mote.RequestDownstreamFrameSequenceNumber();
		}

		ValidValueUint16 const& Token() const {return token;}

		void SequenceNumberGranted(uint32 sequenceNumber)
		{
			bool encrypt = receivedData.Port() != LoRa::macCommandPort;
			LoRa::FrameApplicationData encryptedData;

			if (encrypt)
			{
				encryptedData.SetLength(receivedData.Length());
				encryptedData.Port(receivedData.Port());
				LoRa::EncryptPayload(mote.SessionKey(), receivedData.Data(), receivedData.Length(), mote.NetworkAddress(), false, sequenceNumber, encryptedData.DataNonConst());
			}

			mote.ForwardApplicationData(false, sequenceNumber, token, encrypt ? encryptedData : receivedData);
			Clear();
		}
	};

	EuiType										appEui;
	LoRa::CypherKey								applicationKey;		//the two keys could share the same memory but this is clearer
	LoRa::CypherKey								sessionKey;
	uint32										applicationNonce;	//only used between 'join request' constructor and SendJoinComplete()
	ValidValueUint32							networkAddress;	//of mote
	DownstreamDataController					downstreamDataController;

public:
	Mote(EuiType moteEui, uint32 myNetworkAddress, EuiType myAppEui, LoRa::CypherKey const& myApplicationKey, uint16 myDeviceNonce);	//Used by join request
	Mote(EuiType moteEui, uint32 myNetworkAddress, EuiType myAppEui, LoRa::CypherKey const& myApplicationKey, LoRa::CypherKey const& myEncryptionSessionKey);	//Read from activeMotes database table
	Mote(EuiType moteEui, uint32 myNetworkAddress, EuiType myAppEui, LoRa::CypherKey const& myEncryptionSessionKey);	//Used by auto join and provisioning

	EuiType ApplicationEui() const {return appEui;}
	LoRa::CypherKey const& SessionKey() const {return sessionKey;}
	uint32 NetworkAddress() const {return networkAddress;}

	void ApplicationDataReceived(bool up, ValidValueUint32 const& sequenceNumber, ValidValueUint16 token,
		LoRa::FrameApplicationData const& payload, 
		MoteTransmitRecord const& transmitRecord, GatewayReceiveList const& gatewayReceiveList);

	void RequestDownstreamFrameSequenceNumber();

	void JoinedNetwork(uint16 deviceNonce);
	void SendJoinComplete(uint16 deviceNonce);

	bool Provisioned() const {return Id() <= uint32(UINT32_MAX);}
	bool InNetwork() const {return networkAddress.Valid();}
	void ResetCommand() {ResetDetected();}

	void DataTransmittedToGateway(bool app, uint16 token);	//if app is false, data was a MAC command
	void AcknowledgeReceived(bool app);
	void QueueLengthQueryReceived(bool app);
	void QueueLengthReceived(bool app, uint16 length);
	void SequenceNumberGranted(uint32 grantedNumber) {downstreamDataController.SequenceNumberGranted(grantedNumber);}
	void ResetDetected();
	LoRa::CypherKey const& ApplicationKey() const {return applicationKey;}

private:
	void ForwardApplicationData(bool up, ValidValueUint32 const& sequenceNumber, ValidValueUint16 const& token, LoRa::FrameApplicationData const& payload, MoteTransmitRecord const& transmitRecord, GatewayReceiveList const& gatewayList) const;
	void ForwardApplicationData(bool up, ValidValueUint32 const& sequenceNumber, ValidValueUint16 const& token, LoRa::FrameApplicationData const& payload) const
	{
		MoteTransmitRecord invalidTransmitRecord;
		GatewayReceiveList invalidGatewayList;
		ForwardApplicationData(up, sequenceNumber, token, payload, invalidTransmitRecord, invalidGatewayList);
	}

	void InformUpstreamServerOfMoteReset();

	void UpdateApplicationNonce();

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

	void CreateMote(EuiType moteEui, uint32 myNetworkAddress, EuiType myAppEui, LoRa::CypherKey const& applicationKey, uint16 myDeviceNonce);	//used for join request/response
	void CreateMote(EuiType moteEui, uint32 myNetworkAddress, EuiType myAppEui, LoRa::CypherKey const& myEncryptionSessionKey);

	void ApplicationDataReceived(bool up, EuiType moteEui, ValidValueUint32 const& sequenceNumber, ValidValueUint16 const& token,
		LoRa::FrameApplicationData const& payload, MoteTransmitRecord const& transmitRecord, GatewayReceiveList const& gatewayReceiveList);

	Mote* GetById(EuiType eui) const {return static_cast<Mote*>(BinarySearchVectorNV::List<Mote, EuiType>::GetById(eui));}
	Mote* GetByIndex(uint32 index) const {return static_cast<Mote*>(BinarySearchVectorNV::List<Mote, EuiType>::GetByIndex(index));}
	bool Delete(EuiType eui) {return BinarySearchVectorNV::List<Mote, EuiType>::DeleteById(eui);}

private:
	void Add(Mote* mote) {BinarySearchVectorNV::List<Mote, EuiType>::Add(mote);}
};

#endif

