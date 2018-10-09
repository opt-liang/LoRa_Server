/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef LORAREGION_HPP
#define LORAREGION_HPP

#include "General.h"
#include "LoRa.hpp"
#include "ValueWithValidity.hpp"
#include "TimeRecord.hpp"
#include "Utilities.hpp"
#include <string>


//lint -e758 global enum not referenced
namespace LoRa
{
	namespace Americas902
	{
		const uint32 bandwidthH_Hz = 500 * 1000;
		const DataRate defaultWindow1DataRate(LoRa::loRaMod, bandwidthH_Hz, 12);
	}

	namespace China779
	{
		const DataRate defaultWindow1DataRate(LoRa::loRaMod, 125 * 1000, 12);
	}

	namespace Europe863
	{
		const DataRate defaultWindow1DataRate(LoRa::loRaMod, 125 * 1000, 9);
	}
	
	namespace Europe433
	{
		const DataRate defaultWindow1DataRate(LoRa::loRaMod, 125 * 1000, 9);
	}
	
	uint32 Get2ndWindowDownlinkFrequency_Hz(Region region);
	bool IsValidUplinkFrequency(LoRa::Region region, uint32 uplinkFrequency_Hz, bool highBandwidth);

	inline bool IsValidRegion(Region r) {return r == americas902 || r == china779 || r == europe433 || r == europe863;}
	Region ReadRegion(std::string const& text);
	char const* RegionText(Region region);
	inline uint16 MaxRegionText() {return 12;}

	uint32 GetMinUplinkFrequency_Hz(Region region, bool highBandwidth);
	uint32 GetMaxUplinkFrequency_Hz(Region region, bool highBandwidth);
	bool IsValidUplinkFrequency(LoRa::Region region, uint32 uplinkFrequency_Hz);

	uint32 GetDownLinkFrequency_Hz(LoRa::Region region, uint32 uplinkFrequency_Hz, uint8 transmissionWindow);
	DataRate GetDownlinkDataRate(LoRa::Region region, LoRa::DataRate const& upstreamDataRate, uint8 transmissionWindow);
	CodeRate GetDownlinkCodeRate(CodeRate const& uplinkCodeRate, uint8 transmissionWindow);
	CodeRate const& Get2ndWindowDownlinkCodeRate(Region region);
	uint16 NumberOfSpreadingFactors(Region region, bool highBandwidth);

	uint8 MaxDataRateHeaderValue(Region region, bool highBandwidth);
	uint8 MaxAdrDataRateHeaderValue(Region region);
}


#endif

