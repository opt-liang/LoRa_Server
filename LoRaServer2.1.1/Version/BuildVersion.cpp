/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "BuildVersion.hpp"
#include "Utilities.hpp"

#include <sstream>

#ifndef BUILD_VERSION
	#error BUILD_VERSION not defined
#endif


std::string BuildVersion::Date()
{
	std::stringstream text;
	
	text << __DATE__;

	std::string monthText;
	uint day;
	uint year;
	
	text >> monthText;
	text >> day;
	text >> year;

	sint8 month = FindMonth(monthText);

	if (year < 2014 || month < 1 || month > 12 || day < 1 || day > 31)
		return "";

	return SqlDate(year, uint(month), day);
}


std::string BuildVersion::Time()
{
	std::string timeText = __TIME__;

	uint i =0;
	uint hour = 0;
	for (; isdigit(timeText[i]);i++)
	{
		hour *= 10;
		hour += uint(timeText[i] - '0');
	}

	for (;!isdigit(timeText[i]); i++)
		;	//empty

	uint min = 0;
	for (; isdigit(timeText[i]);i++)
	{
		min *= 10;
		min += uint(timeText[i] - '0');
	}

	for (;!isdigit(timeText[i]); i++)
		;	//empty

	uint sec = 0;
	for (; i < timeText.length() && isdigit(timeText[i]);i++)
	{
		sec *= 10;
		sec += uint(timeText[i] - '0');
	}

	if (hour > 23 || min > 59 || sec > 60) // sec allows for leap second 23:59:60
		return "";

	return SqlTime(hour, min, sec);
}


char const* BuildVersion::VersionString()
{
	static char const result[] = BUILD_VERSION;
	return result;
}

