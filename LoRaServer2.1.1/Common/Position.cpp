/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "Position.hpp"
#include <sstream>
#include <iomanip>
#include <math.h>

namespace
{
	const std::string spacer = "  ";
}


std::string Position::Text(bool printAltitude, bool printTolerance = false) const
{
	if (!Valid())
		return "";

	char north = latitude.Value() >= 0 ? 'N' : 'S';
	char east = longitude.Value() >= 0 ? 'E' : 'W';

	std::stringstream text;
	text <<
		std::setfill('0') << std::setw(8LL) << std::fixed << std::setprecision(4) << fabs(latitude.Value())  << north << " " <<
		std::setfill('0') << std::setw(8LL) << std::fixed << std::setprecision(4) << fabs(longitude.Value()) << east << " ";

	if (printAltitude && altitude.Valid())
		text << std::setw(9LL) << std::fixed << std::setprecision(1) << altitude << 'm';

	if (printTolerance && ToleranceValid())
		text << spacer << std::setfill(' ') << std::setw(5) << std::fixed << std::setprecision(1) << Tolerance() << 'm';

	return text.str();
}

