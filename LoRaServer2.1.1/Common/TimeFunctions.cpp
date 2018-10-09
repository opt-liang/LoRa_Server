/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "TimeFunctions.hpp"
#include "Mutex.hpp"

namespace
{
	Mutex	timeMutex;
}

struct tm gmtimeSafe(time_t const& rawtime)
{
	MutexHolder holder(timeMutex);
	struct tm result = *gmtime(&rawtime);

	return result;
}

struct tm localtimeSafe (time_t const& rawtime)
{
	MutexHolder holder(timeMutex);
	struct tm result = *localtime(&rawtime);

	return result;
}

time_t mktimeSafe(struct tm const& timeStruct)
{
	MutexHolder holder(timeMutex);
	time_t timeCount = mktime (const_cast<struct tm *>(&timeStruct));	//the documentation says that timeStruct isn't changed

	return timeCount;
}

