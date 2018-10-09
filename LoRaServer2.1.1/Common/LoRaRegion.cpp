/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "LoRaRegion.hpp"
#include <stdlib.h>

namespace LoRa
{
	const CodeRate									moteWindow1CodeRate(4,5);	//2nd window, for all regions;

	namespace Americas902
	{
		const uint32 minFrequencyL_Hz					= 902300000;
		const uint32 maxFrequencyL_Hz					= 914900000;
		const uint32 channelSpacingL_Hz					=    200000;

		const uint32 minFrequencyH_Hz					= 903000000;
		const uint32 maxFrequencyH_Hz					= 914200000;
		const uint32 channelSpacingH_Hz					=   1600000;

		const uint32 downlinkFrequencyWindow1_Hz		= 923300000;

		const uint32 minDnFrequency_Hz					= 923300000;
		const uint32 channelSpacingDl_Hz				=    600000;

		const uint16 numberOfSpreadingFactorsL = 4;
		const uint16 numberOfSpreadingFactorsH = 6;

		const uint32 frequencyMeasurementTolerance_Hz	=     10000;	//+/-10kHz

		uint32 RoundUplinkFrequencyToChannel_Hz(uint32 measuredFrequency_Hz);	//returns measured frequency, rounded to a permitted channel frequency
		uint16 GetMinChannel(bool highBandwidth);


		uint32 GetChannelSpacing_Hz(bool highBandwidth);
		uint32 GetUplinkFrequency_Hz(uint16 channel);
		uint16 FindUplinkChannel(uint32 frequency_Hz);
		uint16 NumberOfUplinkChannels();
		uint16 NumberOfUplinkChannels(bool highBandwidth);	//highBandwidth should never be true if region is not americas902
		//F and C are used to separate the two IsHighBandwidth functions
		bool IsHighBandwidthF(uint32 frequency_Hz);	//if region != americas902 returns true, otherwise returns true if in subband, on nearer to it than lower
		bool IsHighBandwidthC(uint16 channel);

		sint32 OffsetFromChannel_Hz(uint32 frequency_Hz, bool highBandwidth);
		uint32 GetDownlinkFrequency_Hz(uint16 channel);
	}

	namespace China779
	{
		const uint32 minFrequency_Hz					= 779500000;
		const uint32 maxFrequency_Hz					= 786500000;

		const uint32 downlinkFrequencyWindow1_Hz		= 786000000;

		const uint16 numberOfSpreadingFactors = 6;
	}

	namespace Europe863
	{
		const uint32 minFrequency_Hz					= 863000000;
		const uint32 maxFrequency_Hz					= 870000000;

		const uint32 downlinkFrequencyWindow1_Hz		= 869525000;

		const uint16 numberOfSpreadingFactors = 6;
	}
	
	namespace Europe433
	{
		const uint32 minFrequency_Hz					= 433175000;
		const uint32 maxFrequency_Hz					= 434665000;

		const uint32 downlinkFrequencyWindow1_Hz		= 434665000;

		const uint16 numberOfSpreadingFactors = 6;
	}
	
	uint32 Get2ndWindowDownlinkFrequency_Hz(Region region);
	bool IsValidUplinkFrequency(LoRa::Region region, uint32 uplinkFrequency_Hz, bool highBandwidth);

	DataRate const  Get1stWindowDownlinkDataRate(Region region, LoRa::DataRate const& upstreamDataRate);	//return by value not reference
}

LoRa::Region LoRa::ReadRegion(std::string const& text)
{
	for (uint i = 0; i < numberOfRegions; i++)
	{
		if (CaseInsensitiveEqual(text, RegionText(Region(i))))
			return Region(i);
	}
	return unknownRegion;
}


char const* LoRa::RegionText(LoRa::Region r)
{
	switch(r)
	{
	case LoRa::americas902:	return "americas902";
	case LoRa::china779:	return "china779";
	case LoRa::europe433:	return "europe433";
	case LoRa::europe863:	return "europe863";
	default:				return "unknownRegion";
	}
}


uint32 LoRa::GetMinUplinkFrequency_Hz(LoRa::Region region, bool highBandwidth)
{
	switch (region)
	{
	case americas902:
						if (highBandwidth)
							return Americas902::minFrequencyH_Hz;
						else
							return Americas902::minFrequencyL_Hz;

	case china779:		return China779::minFrequency_Hz;
	case europe433:		return Europe433::minFrequency_Hz;
	case europe863:		return Europe863::minFrequency_Hz;
	default:			return 0;
	}
}


uint32 LoRa::GetMaxUplinkFrequency_Hz(Region region, bool highBandwidth)
{
	switch (region)
	{
	case americas902:
						if (highBandwidth)
							return Americas902::maxFrequencyH_Hz;
						else
							return Americas902::maxFrequencyL_Hz;

	case china779:		return China779::maxFrequency_Hz;
	case europe433:		return Europe433::maxFrequency_Hz;
	case europe863:		return Europe863::maxFrequency_Hz;
	default:			return 0;
	}
}


uint16 LoRa::Americas902::FindUplinkChannel(uint32 frequency_Hz)
{
	bool highBandwidth = IsHighBandwidthF(frequency_Hz);

	uint32 channelSpacing = GetChannelSpacing_Hz(highBandwidth);

	uint32 minFrequency = GetMinUplinkFrequency_Hz(americas902, highBandwidth);
	uint32 offsetFromMin = frequency_Hz - minFrequency;

	uint16 channel = uint16(offsetFromMin / channelSpacing);	//always selects the channel below the frequency

	return channel;
}


uint32 LoRa::Americas902::RoundUplinkFrequencyToChannel_Hz(uint32 measuredFrequency_Hz)	//returns measured frequency, rounded to a permitted channel frequency
{
	uint16 channel = FindUplinkChannel(measuredFrequency_Hz);

	return GetUplinkFrequency_Hz(channel);
}


uint32 LoRa::Americas902::GetChannelSpacing_Hz(bool highBandwidth)
{
	return highBandwidth ? channelSpacingH_Hz : channelSpacingL_Hz;
}


uint32 LoRa::Americas902::GetUplinkFrequency_Hz(uint16 channel)
{
	bool highBandwidth = IsHighBandwidthC(channel);
	uint32 channelSpacing = GetChannelSpacing_Hz(highBandwidth);
	uint32 minFrequency = GetMinUplinkFrequency_Hz(americas902, highBandwidth);

	uint16 minChannel = GetMinChannel(highBandwidth);
	uint16 correctedChannel = channel - minChannel;
	uint32 result = correctedChannel * channelSpacing + minFrequency;
	return result;
}


uint32 LoRa::Americas902::GetDownlinkFrequency_Hz(uint16 channel)
{
	return Americas902::minDnFrequency_Hz + Americas902::channelSpacingDl_Hz * channel;
}


bool LoRa::Americas902::IsHighBandwidthF(uint32 frequency_Hz)
{
	return IsValidUplinkFrequency(americas902, frequency_Hz, true);
}


bool LoRa::Americas902::IsHighBandwidthC(uint16 channel)
{
	return channel >= NumberOfUplinkChannels(false);
}


uint16 LoRa::Americas902::NumberOfUplinkChannels()
{
	return NumberOfUplinkChannels(false) + NumberOfUplinkChannels(true);
}


uint16 LoRa::Americas902::NumberOfUplinkChannels(bool highBandwidth)
{
	if (highBandwidth)
		return (maxFrequencyH_Hz - minFrequencyH_Hz) / channelSpacingH_Hz + 1;
	else
		return (maxFrequencyL_Hz - minFrequencyL_Hz) / channelSpacingL_Hz + 1;
}


uint16 LoRa::NumberOfSpreadingFactors(Region region, bool highBandwidth)
{
	switch(region)
	{
	case LoRa::americas902:
		if (highBandwidth)
			return Americas902::numberOfSpreadingFactorsH;
		else
			return Americas902::numberOfSpreadingFactorsL;

	case LoRa::china779:		return China779::numberOfSpreadingFactors;
	case LoRa::europe433:		return Europe433::numberOfSpreadingFactors;
	case LoRa::europe863:		return Europe863::numberOfSpreadingFactors;
	default:					return 1;	//should never be called
	}
}


sint32 LoRa::Americas902::OffsetFromChannel_Hz(uint32 frequency_Hz, bool highBandwidth)
{
	uint32 channelSpacing = GetChannelSpacing_Hz(highBandwidth);

	uint32 minFrequency = GetMinUplinkFrequency_Hz(americas902, highBandwidth);
	uint32 offsetFromMin = frequency_Hz - minFrequency;

	uint16 channel = uint16(offsetFromMin / channelSpacing);	//always selects the channel below the frequency

	sint32 offsetFromChannel = offsetFromMin % channelSpacing;

	// if the offset from the channel below is greater than half the channel spacing and the channel blow is not the highest one
	if (uint32(labs(offsetFromChannel)) > channelSpacing/ 2 && (channel + 1) < NumberOfUplinkChannels(highBandwidth))
		offsetFromChannel -= channelSpacing;

	return offsetFromChannel;
}


bool LoRa::IsValidUplinkFrequency(LoRa::Region region, uint32 uplinkFrequency_Hz, bool highBandwidth)
{
	if (uplinkFrequency_Hz < GetMinUplinkFrequency_Hz(region, highBandwidth))
		return false;

	if (uplinkFrequency_Hz > GetMaxUplinkFrequency_Hz(region, highBandwidth))
		return false;

	if (region == americas902)
	{
		sint32 offsetFromChannel = Americas902::OffsetFromChannel_Hz(uplinkFrequency_Hz, highBandwidth);

		return abs(offsetFromChannel) <= Americas902::frequencyMeasurementTolerance_Hz;
	}

	return true;
}


bool LoRa::IsValidUplinkFrequency(LoRa::Region region, uint32 uplinkFrequency_Hz)
{
	if (IsValidUplinkFrequency(region, uplinkFrequency_Hz, false))
		return true;

	if (region != americas902)
		return false;

	return IsValidUplinkFrequency(region, uplinkFrequency_Hz, true);
}



uint32 LoRa::GetDownLinkFrequency_Hz(LoRa::Region region, uint32 uplinkFrequency_Hz, uint8 transmissionWindow)
{
	if (transmissionWindow == 0)
	{
		if (region != LoRa::americas902)
			return uplinkFrequency_Hz;
		else
		{
			uint32 roundedUplinkFrequency_Hz = Americas902::RoundUplinkFrequencyToChannel_Hz(uplinkFrequency_Hz);
			uint16 uplinkChannel = Americas902::FindUplinkChannel(roundedUplinkFrequency_Hz);

			uint16 downlinkChannel = uplinkChannel % 8;

			return Americas902::GetDownlinkFrequency_Hz(downlinkChannel);
		}
	}
	else
		return Get2ndWindowDownlinkFrequency_Hz(region);
}


uint32 LoRa::Get2ndWindowDownlinkFrequency_Hz(LoRa::Region region)
{
	switch (region)
	{
	case LoRa::americas902:		return LoRa::Americas902::downlinkFrequencyWindow1_Hz;
	case LoRa::china779:		return LoRa::China779::downlinkFrequencyWindow1_Hz;
	case LoRa::europe433:		return LoRa::Europe433::downlinkFrequencyWindow1_Hz;
	case LoRa::europe863:		return LoRa::Europe863::downlinkFrequencyWindow1_Hz;
	default:					return 0;
	}
}


LoRa::CodeRate LoRa::GetDownlinkCodeRate(CodeRate const& uplinkCodeRate, uint8 transmissionWindow)
{
	return (transmissionWindow == 0) ? uplinkCodeRate : moteWindow1CodeRate;
}


LoRa::CodeRate const& LoRa::Get2ndWindowDownlinkCodeRate(LoRa::Region region)
{
	return moteWindow1CodeRate;
}


uint16 LoRa::Americas902::GetMinChannel(bool highBandwidth)
{
	if (!highBandwidth)
		return 0;
	else
		return NumberOfUplinkChannels(false);
}


uint8 LoRa::TranslateDataRateToDRFrameHeaderNibble(LoRa::Region region, LoRa::DataRate const& dataRate, bool downstream, bool highBandwidth)
{
	if (region != LoRa::americas902)
	{
		if (dataRate.bandwidth_Hz == 250000 && dataRate.spreadingFactor == 8)
			return 6;

		switch (dataRate.spreadingFactor.Value())
		{
		case  7:	return 5;
		case  8:	return 4;
		case  9:	return 3;
		case 10:	return 2;
		case 11:	return 1;
		default:	return 0;
		}
	}
	else
	{
		if (highBandwidth)
		{
			switch(dataRate.spreadingFactor.Value())
			{
			case 12:	return 8;
			case 11:	return 9;
			case 10:	return 10;
			case  9:	return 11;
			case  8:	return 12;
			case  7:	return 13;
			default:	return 8;	//return 8, most robust high bandwidth spreading factor
			}
		}
		else
		{
			switch(dataRate.spreadingFactor.Value())
			{
			case 10:	return 0;
			case  9:	return 1;
			case  8:
				if (downstream && dataRate.bandwidth_Hz >= DataRate::highBandwidthThreshold_Hz)
					return 4;
				else
					return 2;
			case  7:	return 3;
			default:	return 0;
			}
		}
	}
}


LoRa::DataRate LoRa::TranslateFrameHeaderNibbleToDataRate(LoRa::Region region, uint8 dataRateNibble)
{
	//The operation of this function is specficied in LoRaWAN specification Section 7
	DataRate result;

	result.modulation = LoRa::loRaMod;
	if (region != americas902)
	{
		if (dataRateNibble == 7)
		{
			//FSK
			result.modulation = LoRa::fskMod;
			result.bandwidth_Hz = 50000;
			return result;
		}
		else
		{
			result.bandwidth_Hz = 125 * 1000;
		
			switch (dataRateNibble)
			{
			case 6:		result.spreadingFactor = static_cast<LoRa::SpreadingFactor>(7); result.bandwidth_Hz = 250 * 1000;	break;
			case 5:		result.spreadingFactor = static_cast<LoRa::SpreadingFactor>(7);		break;
			case 4:		result.spreadingFactor = static_cast<LoRa::SpreadingFactor>(8);		break;
			case 3:		result.spreadingFactor = static_cast<LoRa::SpreadingFactor>(9);		break;
			case 2:		result.spreadingFactor = static_cast<LoRa::SpreadingFactor>(10);	break;
			case 1:		result.spreadingFactor = static_cast<LoRa::SpreadingFactor>(11);	break;
			default:	result.spreadingFactor = static_cast<LoRa::SpreadingFactor>(12);	break;
			}
		}
	}
	else
	{
		//Americas
		result.bandwidth_Hz = (dataRateNibble < 4) ? (125 * 1000) : (500 * 1000);

		switch (dataRateNibble)
		{
		case  0:	result.spreadingFactor = static_cast<LoRa::SpreadingFactor>(10);	break;
		case  1:	result.spreadingFactor = static_cast<LoRa::SpreadingFactor>(9);		break;
		case  2:	result.spreadingFactor = static_cast<LoRa::SpreadingFactor>(8);		break;
		case  3:	result.spreadingFactor = static_cast<LoRa::SpreadingFactor>(7);		break;
		case  4:	result.spreadingFactor = static_cast<LoRa::SpreadingFactor>(8);		break;

		case  8:	result.spreadingFactor = static_cast<LoRa::SpreadingFactor>(12);	break;
		case  9:	result.spreadingFactor = static_cast<LoRa::SpreadingFactor>(11);	break;
		case 10:	result.spreadingFactor = static_cast<LoRa::SpreadingFactor>(10);	break;
		case 11:	result.spreadingFactor = static_cast<LoRa::SpreadingFactor>(9);		break;
		case 12:	result.spreadingFactor = static_cast<LoRa::SpreadingFactor>(8);		break;
		case 13:	result.spreadingFactor = static_cast<LoRa::SpreadingFactor>(7);		break;

		default:	result.spreadingFactor = static_cast<LoRa::SpreadingFactor>(10);	break;
		}
	}

	return result;
}


uint8 LoRa::MaxDataRateHeaderValue(LoRa::Region region, bool highBandwidth)
{
	switch (region)
	{
	case LoRa::americas902:
		if (highBandwidth)
			return 13;
		else
			return 4;

	case LoRa::china779:
	case LoRa::europe433:
	case LoRa::europe863:
	default:
		return 7;
	}
}


uint8 LoRa::MaxAdrDataRateHeaderValue(Region region)
{
	switch (region)
	{
	case LoRa::americas902:
		return 4;

	case LoRa::china779:
	case LoRa::europe433:
	case LoRa::europe863:
	default:
		return 7;
	}
}

