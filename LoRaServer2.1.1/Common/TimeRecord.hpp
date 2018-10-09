/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef TIMERECORD_HPP
#define TIMERECORD_HPP

#include "General.h"
#include <string>

class TimeRecord
{
	friend int operator==(TimeRecord const& l, TimeRecord const& r);
	friend int operator>(TimeRecord const& l, TimeRecord const& r);

	uint	year;	//4 digit
	uint	month;	//1-12
	uint	day;	//1-31
	uint	hour;	//0-23
	uint	minute;	//0-59
	uint	sec;	//0-60
	uint32	nsec;	//nanoseconds
	bool	accurate;	//accurate within the context

public:
	/*
	loRaMote format    "2013-03-31T16:21:17.528002Z"  ISO 8601 'compact' format
	loRaGateway format "2014-03-31 16:21:17 GMT"      ISO 8601 'expanded' format
	*/
	static uint16 const minTimeTextCharacters = 12; //12 is min number of characters that allow all fields to be held in a string
	static uint32 const millisecondsInSecond = 1000;
	static uint32 const microsecondsInSecond = millisecondsInSecond * 1000;
	static uint32 const nanosecondsInSecond = microsecondsInSecond * 1000;

	//lint --e{1401}  Warning -- member not initialized by constructor
	TimeRecord() : year(0), month(0), day(0), hour(0), minute(0), sec(0), nsec(0), accurate(false) {}
	//lint --e{1401, 1566}  Warning -- member not initialized by constructor
	TimeRecord(char const text[]) 
		: year(0), month(0), day(0), hour(0), minute(0), sec(0), nsec(0), accurate(false)
	{*this = text;}

	int operator=(char const text[]);
	int operator=(struct tm const& timeStruct);
	operator tm() const;  // NOTE tm_wday, tm_yday and tm_isdst are always zero
	operator time_t() const;  // NOTE tm_wday, tm_yday and tm_isdst are always zero
	std::string SQLString() const {return SQLString(*this);}
	std::string JsonString(bool nanoSecondPrecision = false) const;	// if nanoSecondPrecision is false, microsecond precision is used

	void SetToSystemTime(); //GMT
	void Accurate(bool myAccurate) {accurate = myAccurate;}
	bool Accurate() const {return accurate;}

	uint Year() const {return year;}
	uint Month() const {return month;}
	uint Day() const {return day;}
	uint Hour() const {return hour;}
	uint Minute() const {return minute;}
	uint Sec() const {return sec;}
	uint USec() const {return nsec / 1000;}
	uint NSec() const {return nsec;}

	bool Valid() const {return year != 0;}

	static std::string SQLString(TimeRecord const& in);

	void AddNanoseconds(sint32 add);
};

inline int operator==(TimeRecord const& l, TimeRecord const& r)
{
	if (l.nsec != r.nsec)			return 0;
	if (l.sec != r.sec)				return 0;
	if (l.minute != r.minute)		return 0;
	if (l.hour != r.hour)			return 0;
	if (l.day != r.day)				return 0;
	if (l.month != r.month)			return 0;
	if (l.year != r.year)			return 0;

	return 1;
}

inline int operator>(TimeRecord const& l, TimeRecord const& r)
{
	if (l.year > r.year)			return 1;
	if (l.month > r.month)			return 1;
	if (l.day > r.day)				return 1;
	if (l.hour > r.hour)			return 1;
	if (l.minute > r.minute)		return 1;
	if (l.sec > r.sec)				return 1;
	if (l.nsec > r.nsec)			return 1;

	return 0;
}

time_t operator-(TimeRecord const& l, TimeRecord const& r);

inline int operator !=(TimeRecord const& l, TimeRecord const& r) {return !(l == r);}

#endif

