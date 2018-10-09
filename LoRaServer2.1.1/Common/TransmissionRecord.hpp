/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef MOTE_TRANSMISSION_RECORD_HPP
#define MOTE_TRANSMISSION_RECORD_HPP

#include "General.h"
#include "LoRa.hpp"
#include "Eui.hpp"
#include "ValueWithValidity.hpp"
#include "Mutex.hpp"
#include <list>

class MoteTransmitRecord
{
public:
	//If all values are to be set, use the Set() function
	ValidValueUint32			frequency_Hz;
	LoRa::DataRate				dataRate;
	LoRa::CodeRate				codeRate;	//only valid if dataRate.Modulation() == LoRa::loRaMod
	ValidValueBool				adrEnabled;

public:
	MoteTransmitRecord()
	{}

	MoteTransmitRecord(uint32 myFrequency_Hz, 
		LoRa::ModulationType myModulation, LoRa::DataRate const& myDataRate, LoRa::CodeRate const& myCodeRate, bool myAdrEnabled)
		: frequency_Hz(myFrequency_Hz), dataRate(myDataRate), codeRate(myCodeRate), adrEnabled(myAdrEnabled)
	{}

	MoteTransmitRecord(uint32 myFrequency_Hz, LoRa::DataRate const& myDataRate, LoRa::CodeRate const& myCodeRate, bool myAdrEnabled)
		: frequency_Hz(myFrequency_Hz), dataRate(myDataRate), codeRate(myCodeRate), adrEnabled(myAdrEnabled)
	{}

	bool Valid() const {return frequency_Hz.Valid() && dataRate.Valid() && (Modulation() == LoRa::fskMod || codeRate.Valid());}

	void Set(uint32 myFrequency_Hz, LoRa::DataRate const& myDataRate, LoRa::CodeRate const& myCodeRate, bool myAdrEnabled);
	LoRa::ModulationType Modulation() const {return dataRate.Modulation();}
};


class ReceivedMoteTransmitRecord : public MoteTransmitRecord
{
private:
	bool							valid;
	sint16							signalQuality_cB;
	EuiType							gateway;		//gateway from which this information was received

public:
	ReceivedMoteTransmitRecord()
		:valid(false), signalQuality_cB(SHRT_MIN), gateway(invalidEui)
	{}

	void Update(LoRa::FrameCopyType frameCopyType, EuiType myGateway, sint16 mySignalQuality_cB, uint32 myFrequency, LoRa::DataRate const& myDataRate, LoRa::CodeRate const& myCodeRate, bool myAdrEnabled)
	{
		if (frameCopyType != LoRa::duplicate)
			valid = false;

		if (valid)
		{
			if (gateway != myGateway)
				return;

			if (signalQuality_cB >= mySignalQuality_cB)
				return;
		}
		else
		{
			valid = true;
			gateway = myGateway;
		}

		signalQuality_cB = mySignalQuality_cB;

		Set(myFrequency, myDataRate, myCodeRate, myAdrEnabled);
	}
};

class GatewayReceiveRecord
{
public:
	ValueWithValidity<EuiType>		eui;
	TimeRecord						receiveTime;
	ValidValueUint32				receiveTimestamp_us;	//NOT related to any recognised timescale
	ValidValueUint16				channel;
	ValidValueUint16				rfChain;
	ValidValueSint16				signalToNoiseRatio_cB;
	ValidValueSint16				signalStrength_cBm;

	GatewayReceiveRecord()
	{}

	GatewayReceiveRecord(EuiType myEui, 
		TimeRecord const& myReceiveTime, 
		ValidValueUint32 const& myReceiveTimestamp_us,	//NOT related to any recognised timescale
		ValidValueUint16 const& myChannel,
		ValidValueUint16 const& myRxRfChain,
		ValidValueSint16 const& mySignalToNoiseRatio_cB,
		ValidValueSint16 const& mySignalStrength_cBm)
		: eui(myEui)
	{
		Set(myReceiveTime, myReceiveTimestamp_us, myChannel, myRxRfChain, mySignalToNoiseRatio_cB, mySignalStrength_cBm);
	}

	GatewayReceiveRecord(GatewayReceiveRecord const& other)
		: eui(other.Eui())
	{
		Set(other.receiveTime, other.receiveTimestamp_us, other.channel, other.rfChain, other.signalToNoiseRatio_cB, other.signalStrength_cBm);
	}
	
	EuiType Eui() const {return eui;}

	void Set(EuiType newEui, 
		TimeRecord const& myReceiveTime, 
		ValidValueUint32 const& newReceiveTimestamp_us,	//NOT related to any recognised timescale - may not be valid
		ValidValueUint16 const& myChannel,
		ValidValueUint16 const& myRxRfChain,
		ValidValueSint16 const& mySignalToNoiseRatio_cB,
		ValidValueSint16 const& mySignalStrength_cBm)
	{
		eui = newEui;
		Set(myReceiveTime, newReceiveTimestamp_us, myChannel, myRxRfChain, mySignalToNoiseRatio_cB, mySignalStrength_cBm);
	}

	void Set(
		TimeRecord const& myReceiveTime, 
		ValidValueUint32 const& newReceiveTimestamp_us,	//NOT related to any recognised timescale - may not be valid
		ValidValueUint16 const& myChannel,
		ValidValueUint16 const& myRxRfChain,
		ValidValueSint16 const& mySignalToNoiseRatio_cB,
		ValidValueSint16 const& mySignalStrength_cBm)
	{
		receiveTime = myReceiveTime;
		receiveTimestamp_us = newReceiveTimestamp_us;
		channel = myChannel;
		rfChain = myRxRfChain;
		signalToNoiseRatio_cB = mySignalToNoiseRatio_cB;
		signalStrength_cBm = mySignalStrength_cBm;
	}


	ValidValueSint16 SignalQuality_cB() const {return signalToNoiseRatio_cB + signalStrength_cBm;}

	bool Valid() const
	{
		return eui.Valid() && 
			receiveTime.Valid() &&
			//allow receiveTimestamp_us to be invalid
			channel.Valid() && 
			rfChain.Valid();
	}

	GatewayReceiveRecord const& operator=(GatewayReceiveRecord const& other);
};

class GatewayReceiveList
{
public:
	typedef std::list<GatewayReceiveRecord*> List;
private:
	mutable Mutex						mutex;
	List								list;

public:
	GatewayReceiveList() {}
	GatewayReceiveList(GatewayReceiveList const& other);

	//lint -e{1509} (Warning -- base class destructor for class 'list' is not virtual)
	virtual ~GatewayReceiveList()
	{
		MutexHolder holder(mutex);
		while (!list.empty())
		{
			GatewayReceiveRecord* record = list.front();
			list.pop_front();

			delete record;
		}
	}

	bool Add(EuiType myEui, 
		TimeRecord const& myReceiveTime, 
		ValidValueUint32 const& myReceiveTimestamp_us,	//NOT related to any recognised timescale
		ValidValueUint16 const& myChannel,
		ValidValueUint16 const& myRxRfChain,
		ValidValueSint16 const& mySignalToNoiseRatio_cB,
		ValidValueSint16 const& mySignalStrength_cBm)
	//Returns true when previous record was a ghost frame
	{
		GatewayReceiveRecord* gateway = new GatewayReceiveRecord(myEui, myReceiveTime, myReceiveTimestamp_us, myChannel, myRxRfChain, mySignalToNoiseRatio_cB, mySignalStrength_cBm);

		MutexHolder holder(mutex);
		return AddPrivate(gateway);
	}

	bool Add(GatewayReceiveRecord const& myGatewayRecord)
	//Returns true when previous record was a ghost frame
	{
		GatewayReceiveRecord* gateway = new GatewayReceiveRecord(myGatewayRecord);

		return Add(gateway);
	}

	bool Add(GatewayReceiveRecord* gateway)	//consumes object
	{
		MutexHolder holder(mutex);
		return AddPrivate(gateway);
	}

	TimeRecord ReceiveTime() const;
	uint16 Size() const {return list.size();} 
	bool IsEmpty() const {return Size() == 0;}

	void Lock() const {mutex.Lock();}	// const to allow a const object to be locked
	void Unlock() const {mutex.Unlock();}
	GatewayReceiveList::List::const_iterator IteratorBegin() const {return list.begin();}
	GatewayReceiveList::List::const_iterator IteratorEnd() const {return list.end();}
	EuiType BestGatewayEui() const
	{
		if (Size() > 0)
			return(*IteratorBegin())->Eui();	// requires that head of list be best gateway
		else
			return invalidEui;
	}

private:
	bool AddPrivate(GatewayReceiveRecord* gateway, bool testForGhostFrames = true);	//either frees gateway or stores it in list
	/*If testForGhostFrames is true and the list contains a gateway and the previous frame's signal quality was worse, the function returns false */

	/* A ghost frame occurs when a single transmission is received twice by a single gateway.
	One is received on the correct frequency.  If the reception is high power another may be 
	received on another frequency because it has forced its way through the receive filters */


};

#endif

