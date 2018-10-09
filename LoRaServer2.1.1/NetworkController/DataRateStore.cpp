/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "DataRateStore.hpp"
#include "LoRaRegion.hpp"

//#define DRS_NEED_TO_PRINT
#ifdef DRS_NEED_TO_PRINT
	#include <iomanip>
#endif


void DataRateStore::Add(uint32 sequenceNumber, ValidValueSint16 const& snr_cB)
{
	sint16 assumed_snr_cB = snr_cB.Valid() ? snr_cB.Value() : LoRa::assumedSnrOfMissingFrames_cB;

	uint16 storeIndex = FindIndex(assumed_snr_cB);

	if (store.empty() || store.back().sequenceNumber != sequenceNumber)
	{
		// add in dummy elements if some sequence numbers are missing
		if (!store.empty())
		{
			uint32 expectedSequenceNumber = store.back().sequenceNumber + 1;

			if (sequenceNumber != expectedSequenceNumber)
			{
				//sequence mismatch
				uint32 missedSequenceNumbers = sequenceNumber - expectedSequenceNumber;	//unsigned so a -ve numbere will seem big - which is wanted 

				if (missedSequenceNumbers >= maxElements)
					EmptyStore();	//includes when sequenceNumber < expectedSequenceNumber
				else
				{
					//Adding missing values
					for(; expectedSequenceNumber != sequenceNumber; expectedSequenceNumber++)
						Add(expectedSequenceNumber, LoRa::assumedSnrOfMissingFrames_cB);	//Recursive call
				}
			}
		}

		while (store.size() >= maxElements)
		{
			bucket[store.front().index]--;
			store.pop();
		}

		// new element
		QueueElement element;
		element.index = storeIndex;
		element.sequenceNumber = sequenceNumber;

		store.push(element);
		bucket[element.index]++;
	}
	else
	{
		// sequence number already in store
		if (store.back().index > storeIndex)
		{
			bucket[store.back().index]--;
			store.back().index = storeIndex;
			bucket[store.back().index]++;
		}
	}
}


LoRa::DataRate DataRateStore::GetIdealDataRate() const
{
	uint count = 0;
	uint i = 0;
	uint const target = maxElements/2 + 1;	//assumes maxElements is odd

	for (; i < LoRa::maxNumberValidAdrDataRatesInAnyRegion; i++)
	{
		count += bucket[i];

		if (count >= target)
			break;
	}

	return FindDR(i);
}

LoRa::DataRate DataRateStore::FindDR(uint16 index) const
{
	LoRa::DataRate result;

	uint8 drHeaderNibble;
	if (index > LoRa::MaxAdrDataRateHeaderValue(region))
		drHeaderNibble = MaxAdrDataRateHeaderValue(region);
	else
		drHeaderNibble = uint8(index);

	result = TranslateFrameHeaderNibbleToDataRate(region, drHeaderNibble);

	return result;
}


uint16 DataRateStore::FindIndex(sint16 snr_cB) const	// finds bucket to increment
{
	sint SNRmargin_cB = snr_cB - LoRa::snrMargin_cB;

	if (SNRmargin_cB < -190)
		return 0;
	else if (SNRmargin_cB < -165)
		return 1;
	else if (SNRmargin_cB < -140)
		return 2;
	else if (SNRmargin_cB < -115)
		return 3;
	else if (SNRmargin_cB < -90)
		return 4;
	else
		return 5;
}


#ifdef DRS_NEED_TO_PRINT
std::string  DataRateStore::Text(EuiType eui) const
{
	static int count;
	std::stringstream out;

	if (eui != 0)
		out << "Data rate store DBG" << count << " Mode " << AddressText(eui) << " Region " << LoRa::RegionText(region) << std::endl;

	count++;
	out << Elements() << " elements" << std::endl;

	for (uint i = 0; i < LoRa::maxNumberValidDataRatesInAnyRegion; i++)
	{
		LoRa::DataRate dataRate = FindDR(i, region);

		out << std::setw(15) << std::string(dataRate) << (i + LoRa::minSpreadingFactor) << " " << bucket[i] << "  ";
	}

	out << std::endl;
	return out.str();
}

#endif

