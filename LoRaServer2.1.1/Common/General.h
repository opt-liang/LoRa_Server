/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef GENERAL_HPP
#define GENERAL_HPP

#include <limits.h>

#ifdef _MSC_VER
	#pragma warning ( disable : 4514 4512 4710 4355)
#endif

#if USHRT_MAX == 0xffff
	#define INT16TYPE short
#else
	#error Unable to create a 16 bit variable
#endif

#if UINT_MAX == 0xffffffffUL
	#define INT32TYPE int
#elif ULONG_MAX == 0xffffffffUL
	#define INT32TYPE long
#else
	#error Unable to create a 32 bit variable
#endif

#if ULLONG_MAX == 0xffffffffffffffffULL
	#define INT64TYPE long long
#else
	#error Unable to create a 64 bit variable
#endif

typedef signed char			sint8;
typedef unsigned char		uint8;
typedef signed INT16TYPE	sint16;
typedef unsigned INT16TYPE	uint16;
typedef signed INT32TYPE	sint32;
typedef unsigned INT32TYPE	uint32;
typedef signed INT64TYPE	sint64;
typedef unsigned INT64TYPE	uint64;
typedef signed int			sint;  //32 bit - even on 64 bit machines
typedef unsigned int		uint;  //32 bit - even on 64 bit machines

#define UINT16_MAX 0xffff
#define UINT32_MAX 0xffffffff
#define UINT64_MAX 0xffffffffffffffffui64

#define SINT16_MAX 0x7fff
#define SINT16_MIN (-32768)

#define SINT32_MAX 0x7fffffff
#define SINT32_MIN (-2147483647 - 1)


#define memzero(ptr) memset(ptr,0,sizeof(*(ptr)))
#define ArrayZero(ptr,elements) memset(ptr,0,sizeof(*(ptr)) * (elements))

#endif

