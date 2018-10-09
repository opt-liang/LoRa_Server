/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef MOTECS_HPP
#define MOTECS_HPP

#include "General.h"
#include "BinarySearchVectorNV.hpp"
#include "LoRa.hpp"
#include "AppDataQueue.hpp"
#include "TransmissionRecord.hpp"
#include "ExceptionClass.hpp"
#include "Utilities.hpp"
#include "Eui.hpp"
#include "TcpConnectionManager.hpp"

class Mote : public BinarySearchVector::ElementTemplate<Mote, EuiType>
{
private:
	class DownlinkController
	{
	private:
		Mote&			mote;
		AppDataQueue	queue;
		bool			knowsRemoteQueueLength;
		uint16			remoteQueueLength;	// length on a remote server
		uint16			txToken;

	public:
		DownlinkController(Mote& myMote)
			: mote(myMote), knowsRemoteQueueLength(false), remoteQueueLength(0), txToken(0)
		{}

		void SendAppData(LoRa::FrameApplicationData const& data)
		{
			if (knowsRemoteQueueLength && (remoteQueueLength == 0))
				SendToMote(data);
			else
			{
				queue.Add(data);
				RequestQueueLength();
			}
		}

		void RemoteQueueLengthReceived(uint16 length)
		{
			remoteQueueLength = length;
			knowsRemoteQueueLength = true;

			if (queue.Waiting() && (remoteQueueLength == 0))
				SendToMote();
		}

		void PayloadSentToMote(uint16 rxToken)
		{
			if (rxToken == txToken)
			{
				if (knowsRemoteQueueLength && remoteQueueLength > 0)
					remoteQueueLength--;

				if (queue.Waiting() && (remoteQueueLength == 0))
					SendToMote();
			}
			else
			{
				knowsRemoteQueueLength = false;
				if (queue.Waiting())
					RequestQueueLength();
			}
		}
	private:
		void SendToMote()
		{
			SendToMote(queue.HeadOfQueue());
			queue.RemoveHead();
			remoteQueueLength++;
		}
		void SendToMote(LoRa::FrameApplicationData const& frame);	// only called by SendToMote()
		void RequestQueueLength();
	};

public:
	static const EuiType			defaultApplicationEui = 0;

private:
	EuiType							appEui;
	DownlinkController				downlinkController;

public:
	Mote(EuiType myMoteEui, EuiType myAppEui)
		:BinarySearchVector::ElementTemplate<Mote, EuiType>(myMoteEui),
		appEui(myAppEui), downlinkController(*this)
	{}

	EuiType ApplicationEui() const {return appEui;}

	void ApplicationDataReceived(bool up, ValidValueUint32 const& sequenceNumber, ValidValueUint16 token,
		LoRa::FrameApplicationData const& payload, 
		MoteTransmitRecord const& transmitRecord, GatewayReceiveList const& gatewayReceiveList);

	void JoinedNetwork(uint16 deviceNonce);
	void AppDataTransmittedToGateway(uint16 token) {downlinkController.PayloadSentToMote(token);}
	void AcknowledgeReceived();
	void QueryQueueLength();
	void QueueLengthReceived(uint16 length) {downlinkController.RemoteQueueLengthReceived(length);}
	void ResetDetected();

	void SendAppData(LoRa::FrameApplicationData const& data) {downlinkController.SendAppData(data);}	//returns true on success

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

	void CreateMote(EuiType myEui, EuiType myApp);

	void ApplicationDataReceived(bool up, EuiType moteEui, ValidValueUint32 const& sequenceNumber, ValidValueUint16 const& token,
		LoRa::FrameApplicationData const& payload, MoteTransmitRecord const& transmitRecord, GatewayReceiveList const& gatewayReceiveList);

	Mote* GetById(EuiType eui) const {return static_cast<Mote*>(BinarySearchVectorNV::List<Mote, EuiType>::GetById(eui));}
	bool Delete(EuiType eui) {return  BinarySearchVectorNV::List<Mote, EuiType>::DeleteById(eui);}

	void AppDataTransmittedToGateway(EuiType eui, uint16 token);
	void AcknowledgeReceived(EuiType eui);
	void QueueLengthQueryReceived(EuiType eui);
	void QueueLengthReceived(EuiType eui, uint16 length);
	void MoteResetDetected(EuiType eui);	//returns true if mote found


	bool SendAppData(EuiType moteEui, LoRa::FrameApplicationData const& data);	//returns true on success

private:
	void Add(Mote* mote) {BinarySearchVectorNV::List<Mote, EuiType>::Add(mote);}

};

#endif

