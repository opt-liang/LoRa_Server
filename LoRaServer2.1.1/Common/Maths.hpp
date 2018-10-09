/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef MATHS_HPP
#define MATHS_HPP

#include "General.h"
#include <limits.h>

namespace Maths
{
	uint32 Random32(void);	//returns a 32 bit random number 
	uint32 Random32(uint32 range); // returns a random number less than 'range'
	inline bool RandomBool() {return Random32(2) ? true : false;}
	inline float RandomReal() {return float(Random32()) / float(UINT32_MAX);}
	bool InitialiseRandom();	//must be called before Random32() is called
}

#endif

