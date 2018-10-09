/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef MOTE_ALGORITHM_CONTAINER_HPP
#define MOTE_ALGORITHM_CONTAINER_HPP

//List of include files for child classes of MonitorAlgorithm
#include "AdaptiveDataRateController.hpp"
//#include "TrivialControllerAgorithm.hpp"

#include "WordStore.hpp"
#include <vector>

class Mote;

class MoteAlgorithmContainer
{
private:
	std::vector<MonitorAlgorithm*>		algorithms;

public:
	MoteAlgorithmContainer(Mote& myMote)	//myMote is the containing mote
	{
		bool enableByDefault = true;
		uint16 algorithmNumber = 0;	//not globally significant

		//The add function must be called for each Algorithm used
		Add(new AdaptiveDataRateController(myMote, algorithmNumber++, enableByDefault));
//		Add(new TrivialControllerAgorithm(myMote, algorithmNumber++, enableByDefault));
	}

	~MoteAlgorithmContainer(void)
	{
		//delete from end; it's more efficient
		for (int i = algorithms.size() - 1; i >= 0; i--)
		{
			delete algorithms[i];

			algorithms.pop_back();
		}
	}

	void NewData(uint32 sequenceNumber, MoteTransmitRecord const& transmitRecord, GatewayReceiveList const& gatewayReceiveList)
	{
		uint size = algorithms.size();
		for (uint i = 0; i < size; i++)
			algorithms[i]->NewData(sequenceNumber, transmitRecord, gatewayReceiveList);
	}

	void ReminderReceived(uint16 algorithmId)
	{
		if (algorithmId >= algorithms.size())
			return;

		algorithms[algorithmId]->ReminderReceived();
	}

	bool CommandReceived(WordStore& wordStore)
	{
		std::string const& algorithmName = wordStore.GetNextWord();

		uint size = algorithms.size();
		for (uint i = 0; i < size; i++)
		{
			if (algorithmName == algorithms[i]->AlgorithmName())
			{
				algorithms[i]->CommandReceived(wordStore);
				return true;
			}
		}
		return false;
	}

private:

	void Add(MonitorAlgorithm* alg)
	{
		algorithms.push_back(alg);
	}
};

#endif

