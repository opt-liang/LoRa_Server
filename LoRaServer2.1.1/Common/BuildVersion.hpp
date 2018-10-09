/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef BUILD_VERSION_HPP
#define BUILD_VERSION_HPP

#include "General.h"
#include <string>

namespace BuildVersion
{
	const uint16 numberOfVersionLevels = 3;

	std::string Date(void);
	std::string Time(void);

	uint16 Version(uint8 v);

	char const* VersionString();
}

#endif

