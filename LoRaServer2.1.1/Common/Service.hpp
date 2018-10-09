/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef APPLICATION_NAMESPACE_HPP
#define APPLICATION_NAMESPACE_HPP

#include "General.h"
#include <string>

namespace Service
{
	typedef uint32 Mask;

	extern const Mask userDataServer;
	extern const Mask moteTxServer;
	extern const Mask gatewayRxServer;
	extern const Mask joinServer;
	extern const Mask joinMonitor;
	extern const Mask downstreamServer;
	extern const Mask macCommandServer;
	extern const Mask gatewayStatusServer;

	extern const Mask maxMask;

	extern const Mask nullMask;
	extern const Mask fullMask;
	extern const Mask errorMask;

	extern const Mask applicationServerMask;
	extern const Mask customerServerMask;
	extern const Mask networkControllerMask;
	extern const Mask downstreamServerMask;

	inline void SetMaskBits(Mask& mask, bool set, Mask change)
	{
		if (set)
			mask |= change;
		else
			mask &= ~change;
	}

	std::string Text(Mask mask);	//returns text associated with 1 bit of the mask - so only call it with one bit set!!
	std::string TextString(Mask mask);	//returns text associated with all bits of the mask, each separated by a space

	Mask ReadMask(std::string input);
}

#endif

