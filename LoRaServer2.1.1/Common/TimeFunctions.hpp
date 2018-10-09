/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef TIME_FUNCTIONS_HPP
#define TIME_FUNCTIONS_HPP

#include <time.h>

//Each of these functions call their 'raw' standard counterparts (e.g. gmtime) but provide mutex protection
struct tm gmtimeSafe(time_t const& rawtime);
struct tm localtimeSafe (time_t const& rawtime);

time_t mktimeSafe(struct tm const& timeStruct);		//opposite of localtimeSafe

#endif

