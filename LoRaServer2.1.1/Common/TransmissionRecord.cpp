/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "TransmissionRecord.hpp"

void MoteTransmitRecord::Set(uint32 myFrequency_Hz, LoRa::DataRate const& myDataRate, LoRa::CodeRate const& myCodeRate, bool myAdrEnabled)
{
	frequency_Hz = myFrequency_Hz;
	dataRate = myDataRate;
	codeRate = myCodeRate;
	adrEnabled = myAdrEnabled;
}

GatewayReceiveRecord const& GatewayReceiveRecord::operator=(GatewayReceiveRecord const& other)
{
	eui						= other.eui;
	receiveTime				= other.receiveTime;
	receiveTimestamp_us		= other.receiveTimestamp_us;
	channel					= other.channel;
	rfChain					= other.rfChain;
	signalToNoiseRatio_cB	= other.signalToNoiseRatio_cB;
	signalStrength_cBm		= other.signalStrength_cBm;

	return *this;
}


//lint -e{429} (Warning -- Custodial pointer has not been freed or returned)
bool GatewayReceiveList::AddPrivate(GatewayReceiveRecord* gateway, bool testForGhostFrames)
{
	List::iterator search = list.begin();	//iterator
	List::iterator match = list.end();	//search for ghost frame
	List::iterator insert = list.end();	//search for insert location

	for (;search != list.end(); ++search)
	{
		if (testForGhostFrames && gateway->Eui() == (**search).Eui())
		{
			match = search;
			break;
		}

		if (insert == list.end() && gateway->SignalQuality_cB() >= (**search).SignalQuality_cB())
		{
			insert = search;
			if (!testForGhostFrames)
				break;
		}
	}

	if (match != list.end())
	{
		//Ghost found
		if ((**match).SignalQuality_cB() < gateway->SignalQuality_cB())
		{
			// record is better than old record
			list.erase(match);
			AddPrivate(gateway, false);		//recursive call - no need to test for ghost frames
			return true;	// transmit record must be updated
		}
		else
			delete gateway;
	}
	else
	{
		if (search != list.end())
			list.insert(search, gateway);
		else
			list.push_back(gateway);
	}
	return false;
}

GatewayReceiveList::GatewayReceiveList(GatewayReceiveList const& other)
{
	MutexHolder myHolder(mutex);
	MutexHolder otherHolder(other.mutex);

	List::const_iterator it = other.list.begin();

	for (;it != other.list.end(); ++it)
	{
		GatewayReceiveRecord* newElement = new GatewayReceiveRecord(**it);
		AddPrivate(newElement, false);
	}
}


TimeRecord GatewayReceiveList::ReceiveTime() const
{
	TimeRecord const* best = 0;

	MutexHolder holder(mutex);

	if (!list.empty())
	{
		List::const_iterator it = list.begin();

		//lint -e{1702} Info -- operator 'operator!=' is both an ordinary function and a member function
		for (;it != list.end(); ++it)
		{
			TimeRecord const& test = (*it)->receiveTime;

			if (!test.Valid())
				continue;

			else if (!best)
				best = &test;

			else if (best->Accurate() && !test.Accurate())
				best = &test;

			else if (test > *best)	// choose the earliest time
				best = &test;
		}
	}

	if (best)
		return *best;

	TimeRecord invalidResult;
	return invalidResult;
}

