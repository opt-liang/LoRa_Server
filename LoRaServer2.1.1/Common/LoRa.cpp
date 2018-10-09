/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "LoRa.hpp"
#include "Utilities.hpp"

#include <sstream>

namespace LoRa
{
	namespace UDP
	{
		uint16 gatewayMessageProtocolPort = 1700;
		uint16 jsonPort = gatewayMessageProtocolPort + 1;
	}

	namespace TCP
	{
		uint16 jsonPort = UDP::jsonPort;
	}
}

const LoRa::ValidRegion LoRa::invalidRegion;


LoRa::DataRate::operator std::string() const
{
	std::stringstream result;

	if (modulation == LoRa::loRaMod)
	{
		result << "SF";
		result << int(spreadingFactor);
		result << "BW";
		result << int(bandwidth_Hz / 1000);
	}
	else
		result << int(bandwidth_Hz);

	return result.str();
}

LoRa::CodeRate::operator std::string() const
{
	std::stringstream result;

	result << int(carried);
	result << '/';
	result << int(total);

	return result.str();
}

std::string const LoRa::Frame::TypeText() const
{
	std::string result;

	switch (Type())
	{
	case joinRequestFrame:					result = "Join Request Frame";		break;
	case joinAcceptFrame:					result = "Join Accept Frame";		break;
	
	case dataUnconfirmedFrameUp:			result = "Data Unconfirmed Up";		break;
	case dataUnconfirmedFrameDown:			result = "Data Unconfirmed Down";	break;
	case dataConfirmedFrameUp:				result = "Data Confirmed Up";		break;
	case dataConfirmedFrameDown:			result = "Data Confirmed Down";		break;
	default:								result = "Unknown frame type";		break;
	}
	return result;
}


LoRa::DataRate& LoRa::DataRate::operator=(char const text[])
{
	//goto fail is used only to allow the object to be set invalid and the method to return in a single operation
	SetInvalid();

	if (toupper(text[0]) == 'S' && toupper(text[1]) == 'F')
		modulation = LoRa::loRaMod;
	else
		modulation = LoRa::fskMod;

	if (modulation == LoRa::loRaMod)
	{
		int i = 2;

		if (!isdigit(text[i]))
			goto fail;

		unsigned spreadingFactorLocal = 0;
		for (;isdigit(text[i]); i++)
		{
			spreadingFactorLocal *= 10;
			spreadingFactorLocal += text[i] - '0';
		}
	
		if (spreadingFactorLocal > unsigned(LoRa::maxSpreadingFactorInAnyRegion))
			goto fail;

		if (spreadingFactorLocal < unsigned(LoRa::minSpreadingFactor))
			goto fail;

		//lint --e{679} (Warning -- Suspicious Truncation in arithmetic expression combining with pointer)
		if ((toupper(text[i]) != 'B') ||
			(toupper(text[i+1]) != 'W') ||
			(!isdigit(text[i+2])))
			goto fail;

		i += 2;
		uint32 modulationBandwidth_kHz = 0;
		for (; isdigit(text[i]); i++)
		{
			//lint --e{571}  (Warning -- Suspicious cast)
			modulationBandwidth_kHz *= 10;
			modulationBandwidth_kHz += uint32((text[i]) - '0');
		}

		if ((text[i] == 'K') || (text[i] == 'M'))
		{
			uint multiple = 100;	// only multiply by 100 because the next digit will be 1/10th of the unit value
			modulationBandwidth_kHz *= 1000;

			if (text[i] == 'M')
			{
				modulationBandwidth_kHz *= 1000;
				multiple *= 1000;
			}
			i++;

			//lint --e{571}  (Warning -- Suspicious cast)
			for (; isdigit(text[i]) && multiple > 0; i++, multiple /= 10)
				modulationBandwidth_kHz += multiple * uint32((text[i]) - '0');
		}

		if (modulationBandwidth_kHz == 0 || modulationBandwidth_kHz > LoRa::maxDataRate)
			goto fail;

		spreadingFactor = LoRa::SpreadingFactor(spreadingFactorLocal);
		bandwidth_Hz = modulationBandwidth_kHz * 1000;
	}
	else if (modulation == LoRa::fskMod)
	{
		uint32 bandwidthLocal_Hz = ReadUnsignedInteger(text);

		if (bandwidthLocal_Hz == ~uint32(0))
			goto fail;

		bandwidth_Hz = bandwidthLocal_Hz;
	}
	return *this;

fail:
	SetInvalid();
	return *this;
}


LoRa::CodeRate& LoRa::CodeRate::operator=(char const text[])
{
	//goto fail is used only to allow the object to be set invalid and the method to return in a single operation
	int i = 0;
	uint32 carriedRead = ReadUnsignedInteger(&text[i], false, false);
	uint32 totalRead;

	if (carriedRead >= ~uint16(0))
		goto fail;

	while (isdigit(text[i])) i++; //skip over digits

	if (text[i] != '/')
		goto fail;

	i++;
	totalRead = ReadUnsignedInteger(&text[i], false, false);

	if (totalRead >= ~uint16(0))
		goto fail;

	if (carriedRead == 0 || totalRead == 0)
		goto fail;

	carried = uint16(carriedRead);
	total = uint16(totalRead);

	return *this;

fail:
	SetInvalid();
	return *this;
}


LoRa::OptionRecord LoRa::GenerateAdrCommand(uint8 power, uint8 dataRate, uint16 channelMask, uint8 channelMaskControl, uint8 transmissionsOfUnacknowledgedUplinkFrame)
{
	LoRa::OptionRecord command;
	command.SetLength(5);

	command[0] = LoRa::linkAdr;
	command[1] = ((0x0F & dataRate)  << 4) | ((0x0F & power));
	command[2] = static_cast<uint8>(channelMask);		//mask lsb
	command[3] = static_cast<uint8>(channelMask >> 8);	//mask msb
	command[4] = ((0x07 & channelMaskControl) << 4) | (0x0F & transmissionsOfUnacknowledgedUplinkFrame);	//MSB must always be zero

	return command;
}

int operator==(LoRa::DataRate const& l, LoRa::DataRate const& r)
{
	if (!l.Valid() || !r.Valid()) return 0;
	if (l.modulation != r.modulation) return 0;
	if (l.bandwidth_Hz != r.bandwidth_Hz) return 0;
	if (l.spreadingFactor != r.spreadingFactor) return 0;
	return 1;
}


int operator>(LoRa::DataRate const& l, LoRa::DataRate const& r)
{
	if (!l.Valid() || !r.Valid()) return 0;					//can't compare
	if (l.modulation != r.modulation) return 0;				//can't compare	

	if (l.bandwidth_Hz > r.bandwidth_Hz) return 1;
	if (l.spreadingFactor < r.spreadingFactor) return 1;	//if SF is smaller DR is bigger
	return 0;
}

