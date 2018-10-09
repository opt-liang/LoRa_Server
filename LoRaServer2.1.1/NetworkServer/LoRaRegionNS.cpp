#include "LoRaRegion.hpp"
#include "GlobalDataNS.hpp"

namespace LoRa
{
	DataRate const  Get1stWindowDownlinkDataRate(Region region, LoRa::DataRate const& upstreamDataRate);	//return by value not reference
	DataRate const& Get2ndWindowDownlinkDataRate(Region region);
}

LoRa::DataRate LoRa::GetDownlinkDataRate(LoRa::Region region, LoRa::DataRate const& upstreamDataRate, uint8 transmissionWindow)
{
	if (transmissionWindow == 0)
		return Get1stWindowDownlinkDataRate(region, upstreamDataRate);
	else
		return Get2ndWindowDownlinkDataRate(region);
}

LoRa::DataRate const LoRa::Get1stWindowDownlinkDataRate(LoRa::Region region, LoRa::DataRate const& upstreamDataRate)
{
	if (region != americas902)
		return upstreamDataRate;
	else
	{
		LoRa::DataRate result = upstreamDataRate;
		result.bandwidth_Hz = Americas902::bandwidthH_Hz;

		if ((upstreamDataRate.spreadingFactor == 8) && (upstreamDataRate.bandwidth_Hz == Americas902::bandwidthH_Hz))
			result.spreadingFactor = static_cast<SpreadingFactor>(7);	//exception to the general rule

		return result;
	}
}



LoRa::DataRate const& LoRa::Get2ndWindowDownlinkDataRate(LoRa::Region region)
{
	switch (region)
	{
	case LoRa::americas902:		return Global::dataRateWindow1Americas902;
	case LoRa::china779:		return Global::dataRateWindow1China779;
	case LoRa::europe433:		return Global::dataRateWindow1Eu433;
	case LoRa::europe863:		return Global::dataRateWindow1Eu863;
	default:					return Global::dataRateWindow1Eu863;
	}
}

