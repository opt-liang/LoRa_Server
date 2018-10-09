/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef DATA_RATE_STORE_HPP
#define DATA_RATE_STORE_HPP

#include "General.h"
#include "LoRa.hpp"

#include <queue>

class DataRateStore
{
public:

	typedef struct
	{
		uint16			index;
		uint32			sequenceNumber;
	} QueueElement;

private:

	uint16							maxElements;
	sint16							margin_cB;
	std::queue<QueueElement>		store;
	LoRa::ValidRegion				region;

	uint							bucket[LoRa::maxNumberValidAdrDataRatesInAnyRegion];	//count of occurances of each spreadingFactor_ZeroBased in queue

public:
		//lint --e{1566} Warning -- member 'SpreadingFactorStore::bucket' (line 24) might have been initialized by a separate function but no '-sem(SpreadingFactorStore::EmptyStore,initializer)' was seen
	DataRateStore(uint16 myMaxElements, sint16 myMargin_cB)
		: maxElements(myMaxElements), margin_cB(myMargin_cB)
	{
		EmptyStore();
	}

	void Add(uint32 sequenceNumber, ValidValueSint16 const& snr_cB);
	LoRa::DataRate GetIdealDataRate() const;	// for the median SNR of the store
	uint16 Elements() const {return uint16(store.size());}
	bool Valid() const {return Elements() > (maxElements / 2);}
	void SetRegion(LoRa::Region r) {region = r;}
	LoRa::ValidRegion const& GetRegion() const {return region;}

	void EmptyStore() 
	{
		while (!store.empty())
				store.pop();

		for (uint i = 0; i < LoRa::maxNumberValidAdrDataRatesInAnyRegion; i++)
			bucket[i] = 0;
	}

	std::string Text(EuiType eui) const;	//returns a string containing content of store - debug only (define DRS_NEED_TO_PRINT)

private:
	LoRa::DataRate FindDR(uint16 index) const;
	uint16 FindIndex(sint16 snr_cB) const;	// finds bucket to increment

};

#endif

