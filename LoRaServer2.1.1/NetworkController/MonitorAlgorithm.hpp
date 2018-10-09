/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef MONITOR_ALGORITHM_HPP
#define MONITOR_ALGORITHM_HPP

#include "General.h"
#include "TransmissionRecord.hpp"
#include "TcpConnectionManager.hpp"

class Mote;
class MoteTransmitRecord;
class GatewayReceiveList;
class WordStore;

class MonitorAlgorithm
{
protected:
	Mote&			mote;

private:
	uint16 const	algorithmNumber;	//only valid within a single mote
	bool			active;

public:
	MonitorAlgorithm(Mote& myMote, uint16 myAlgorithmNumber, bool myActive) : mote(myMote), algorithmNumber(myAlgorithmNumber), active(myActive) {}
	virtual ~MonitorAlgorithm(void) {}

	bool Active() const {return active;}
	void Active(bool a) {active = a;}

	uint16 AlgorithmNumber() const {return algorithmNumber;}

	//Virtual functions
	virtual void NewData(uint32 sequenceNumber, MoteTransmitRecord const& transmitRecord, GatewayReceiveList const& gatewayReceiveList) = 0;
	virtual bool CommandReceived(WordStore& commandWords) = 0;
	virtual void ReminderReceived() {}	//Must be redefined if the child algorithm wishes to receive ticks

	virtual char const* AlgorithmName() const = 0;	
	/* Must return a value pointer to the algorithm name.  
	Returning a pointer to memory that is common to all elements in the child class will save a lot of memory
	*/
};

#endif

