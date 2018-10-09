/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "LoRa.hpp"
#include "Utilities.hpp"

uint32 LoRa::Read3ByteValue(uint8 const input[])
{
	uint32 result;
	
	result = uint32(input[2]);
	result <<= 8, result |= uint32(input[1]);
	result <<= 8, result |= uint32(input[0]);

	return result;
}


uint32 LoRa::Read4ByteValue(uint8 const input[])
{
	uint32 result;
	
	result = uint32(input[3]);
	result <<= 8, result |= uint32(input[2]);
	result <<= 8, result |= uint32(input[1]);
	result <<= 8, result |= uint32(input[0]);

	return result;
}


uint64 LoRa::Read8ByteValue(uint8 const input[])
{
	uint64 result = Read4ByteValue(input + 4);
	result <<= (4 * 8), result |= Read4ByteValue(input);

	return result;
}


uint8* LoRa::Write1ByteValue(uint8 output[], uint8 input)
{
	uint8* ptr = output;

	*(ptr++) = uint8(input);

	return ptr;
}


uint8* LoRa::Write2ByteValue(uint8 output[], uint16 input)
{
	uint8* ptr = output;

	*(ptr++) = uint8(input), input >>= 8;
	*(ptr++) = uint8(input);

	return ptr;
}

uint8* LoRa::Write3ByteValue(uint8 output[], uint32 input)
{
	uint8* ptr = output;

	*(ptr++) = uint8(input), input >>= 8;
	*(ptr++) = uint8(input), input >>= 8;
	*(ptr++) = uint8(input);

	return ptr;
}


uint8* LoRa::Write4ByteValue(uint8 output[], uint32 input)
{
	uint8* ptr = output;

	*(ptr++) = uint8(input), input >>= 8;
	*(ptr++) = uint8(input), input >>= 8;
	*(ptr++) = uint8(input), input >>= 8;
	*(ptr++) = uint8(input);

	return ptr;
}

uint8* LoRa::Write8ByteValue(uint8 output[], uint64 input)
{
	uint8* ptr = output;
	
	ptr = Write4ByteValue(ptr, uint32(input));
	return Write4ByteValue(ptr, uint32(input >> 32));
}


