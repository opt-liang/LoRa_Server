/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "TimeRecord.hpp"
#include "TimeFunctions.hpp"
#include "Utilities.hpp"
#include <stdio.h>
#include <sstream>
#include <iomanip>

int TimeRecord::operator=(char const text[])
{
	sint valuesRead = sscanf(text,"%u-%u-%u%*c%u:%u:%u",
			&year,&month,&day,&hour,&minute,&sec);

	if (valuesRead < 6)
		return 0;

	nsec = 0;

	//read microseconds
	char const* ptr = text + minTimeTextCharacters;

	//lint -e{722}  (Info -- Suspicious use of ;)
	for (; *ptr != '\0' && *ptr != '\"' && *ptr !=- '}' && *ptr != '.'; ptr++)
		;	//empty for
	
	if (*ptr != '.')
		return 1;	// no decimal point

	//lint -e{690} (Warning -- Possible access of pointer pointing 1 bytes past nul character by operator 'unary *'
	ptr++;	// move past '.';
	uint32 multiple = 100000000; // 100 million

	for (; isdigit(*ptr) && multiple > 0; ptr++, multiple /= 10)
		nsec += uint8(*ptr - '0') * multiple;

	return 1;
}


int TimeRecord::operator=(struct tm const& timeStruct)
{
	year 	= uint(timeStruct.tm_year + 1900);
	month 	= uint(timeStruct.tm_mon + 1);
	day 	= uint(timeStruct.tm_mday);
	hour 	= uint(timeStruct.tm_hour);
	minute 	= uint(timeStruct.tm_min);
	sec		= uint(timeStruct.tm_sec);
	nsec 	= 0;
	accurate = false;
	return 1;
}


TimeRecord::operator tm() const
{
	struct tm result;

	memzero(&result);
	result.tm_year = year - 1900;
	result.tm_mon = month - 1;
	result.tm_mday = day;
	result.tm_hour = hour;
	result.tm_min = minute;
	result.tm_sec = sec;
	return result;
}


TimeRecord::operator time_t() const
{
	struct tm timeStruct = *this;

	time_t result = mktimeSafe(timeStruct);

	return result;
}


std::string TimeRecord::SQLString(TimeRecord const& in)
{
	std::string result = SqlDate(in.year, in.month, in.day);

	result += ' ';
	result += SqlTime(in.hour, in.minute, in.sec);

	return result;
}

void TimeRecord::AddNanoseconds(sint32 nanoseconds)
{
	sint32 rawNanosecondSum = nanoseconds + nsec;

	sint32 secondsAdd = 0;
	if (rawNanosecondSum < 0)
		secondsAdd = -1 - rawNanosecondSum / sint32(TimeRecord::nanosecondsInSecond);
	else
		secondsAdd = rawNanosecondSum / sint32(TimeRecord::nanosecondsInSecond);

	uint32 nanosecondResult = static_cast<uint32>(rawNanosecondSum - secondsAdd * sint32(TimeRecord::nanosecondsInSecond));

	if (secondsAdd != 0)
	{
		struct tm timeStruct = *this;

		time_t timeSecs = mktimeSafe(timeStruct); //converts assuming timeStruct is local time, which it isn't

		timeSecs += secondsAdd;

		timeStruct = localtimeSafe(timeSecs);

		*this = timeStruct;
	}
	nsec = nanosecondResult;	//nsec is zeroed by the cast from struct tm to TimeRecord, so must be written after it
}

std::string TimeRecord::JsonString(bool nanoSecondPrecision) const
{
	std::stringstream result;

	result << std::setfill('0') <<
		std::setw(4) << year << '-' <<
		std::setw(2) << month << '-' <<
		std::setw(2) << day << 'T' <<
		std::setw(2) << hour << ':' <<
		std::setw(2) << minute << ':' <<
		std::setw(2) << sec;
	
	if (accurate)
	{
		uint fractionWidth          = nanoSecondPrecision ? 9 : 6;	//number of digits after decimal point
		uint printedFractionalValue = nanoSecondPrecision ? nsec : (nsec/1000);

		result << '.' << std::setw(fractionWidth) << printedFractionalValue;
	}
	result << 'Z';

	return result.str();
}


void TimeRecord::SetToSystemTime() //GMT
{
	time_t rawtime;
	time(&rawtime);

	struct tm time = gmtimeSafe(rawtime);

	*this = time;
}


time_t operator-(TimeRecord const& l, TimeRecord const& r)
{
	time_t lRaw = l;
	time_t rRaw = r;

	return lRaw - rRaw;
}



