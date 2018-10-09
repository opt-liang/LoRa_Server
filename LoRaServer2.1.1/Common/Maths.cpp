/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "Maths.hpp"
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */


volatile bool dummyRandomNumberInitialisationVariable = Maths::InitialiseRandom(); 
/* variable is only used to force random seed to be initialised.  
volatile to used to ensure the variable is not optimised away */



bool Maths::InitialiseRandom(void)
{
	srand (unsigned(time(NULL)));
	return true;
}


uint32 Maths::Random32(void)
{
	const unsigned fewestRandomBits = 15;	// the specification of rand() requires only 15 random bits
	uint32 result = (uint32(rand()) << 2 * fewestRandomBits) ^ (uint32(rand()) << fewestRandomBits) ^ rand();

	return result;
}

uint32 Maths::Random32(uint32 range)
{
	uint32 result = Random32();

	return result % range;
}

