/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef ADDRESS_RANGE_HPP
#define ADDRESS_RANGE_HPP

#include "General.h"
#include "Eui.hpp"
#include "LoRa.hpp"

class AddressRange
{
private:
	uint32 base;
	uint32 mask;

public:
	AddressRange(uint32 myBase = LoRa::invalidNetworkAddress, uint32 myMask = 0)
		:base(myBase), mask(myMask)
	{}

	void Set(uint32 myBase, uint32 myMask)
	{
		base = myBase;
		mask = myMask;
	}

	bool IsSet() const {return base != LoRa::invalidNetworkAddress;}
	
	bool InRange(uint32 test) const {return (test >= Base()) && (test <= Maximum());}
	uint32 Base() const {return base;}
	uint32 Mask() const {return mask;}
	uint32 Maximum() const {return base | mask;}
};

#endif

