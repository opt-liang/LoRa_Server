/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef TRIVIAL_CONTROLLER_ALGORITHM_HPP
#define TRIVIAL_CONTROLLER_ALGORITHM_HPP

#include "General.h"
#include "MonitorAlgorithm.hpp"

class TrivialControllerAgorithm : public MonitorAlgorithm
{
public:
	TrivialControllerAgorithm(Mote& myMote, uint16 myAlgorithmNumber, bool myActive)
		: MonitorAlgorithm(myMote, myAlgorithmNumber, myActive)
	{}

	//redefined virtual functions from MonitorAlgorithm
	void NewData(uint32 sequenceNumber, MoteTransmitRecord const& transmitRecord, GatewayReceiveList const& gatewayReceiveList);
	char const* AlgorithmName() const {return "tca";}
	bool CommandReceived(WordStore& commandWords);
	//void ReminderReceived();  //uncomment and define if the algorithm needs to be able to be reminded.

	//end of redefined virtual functions
};

#endif

