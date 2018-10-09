/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "Utilities.hpp"

#include <memory.h>
#include <stdio.h>
#include <iomanip>
#include <stdlib.h>
#include <sstream>
#include <cstdlib>
#include <limits.h>

#ifdef _MSC_VER
	#include <Windows.h>
#else
	#include <unistd.h>
	#include <time.h>
#endif

//lint -e1702 (Info -- operator 'operator ??' is both an ordinary function

void ConvertStringToWordVector(std::string const& in, std::vector<std::string>& out)
{
	std::string::const_iterator it = in.begin();

	for (;;)
	{
		while (it < in.end() && isspace(*it))
			++it;

		if (it >= in.end())
			break;

		std::string::const_iterator endWord = it;

		while (endWord < in.end() && !isspace(*endWord))
			++endWord;

		std::string word = in.substr(uint(it - in.begin()),uint(endWord - it));

		if (word[0] == '#')
			break;
		
		out.push_back(word);

		it = endWord;
	}
}

void ConvertStringPointerArrayToWordVector(int elements, char const* in[], std::vector<std::string>& out)
{
	char const** end = in + elements;
	char const** ptr = in;

	for (;ptr < end; ptr++)
	{
		std::string word = *ptr;

		out.push_back(word);
	}
}


std::string FindFirstWord(std::string const& in)
{
	std::string result;
	std::string::const_iterator it = in.begin();

	while (it < in.end() && isspace(*it))
		++it;

	if (it >= in.end())
		return result;	// block line

	std::string::const_iterator endWord = it;

	while (endWord < in.end() && !isspace(*endWord))
		++endWord;

	result = in.substr(uint(it - in.begin()),uint(endWord - it));

	return result;
}

std::string ReplaceFileExtension(std::string const& in, char const newExtension[])
{
	std::string result = in;
	std::size_t dotPosition = result.rfind('.');

	if (dotPosition == std::string::npos)
		result += '.';

	else if (result.length() > dotPosition)
		result.erase(dotPosition+1);	// erase all after final dot

	result += newExtension;

	return result;
}



bool IsBlank(std::string const& in) //should be a member but don't want to create a child class of std::string
{
	std::string::const_iterator it = in.begin();

	while (it < in.end() && isspace(*it))
		++it;

	return it >= in.end();
}


bool IsCommentLine(std::string const& in)
{
	std::string::const_iterator it = in.begin();

	for (;it < in.end();++it)
	{
		if (*it == '#')
			return true;	//first non blank

		if (!isspace(*it))
			return false;
	}
	return false;
}


std::string ConvertIntToHexText(uint64 in, uint8 minCharacters, char fieldSeparator)
{
	std::string result;

	if (in == 0 && minCharacters <= 1)
		result = "0";
	else
	{
		unsigned inserted = 0;

		for (;inserted < minCharacters || in > 0; inserted += 2, in /= 0x100)
		{
			if (inserted > 0 && fieldSeparator != '\0' && (in > 0xFF || inserted < minCharacters))
				result.insert(0,1,fieldSeparator);

			for (unsigned i = 0; i < 2; i++)
			{
				uint8 hexDigit = uint8((i == 0) ? in%0x10 : in/0x10) & 0xF;

				char character;
				if (hexDigit > 9)
					character = char('a' + hexDigit - 0xA);
				else
					character = char('0' + hexDigit);

				result.insert(0,1, character);
			}

		}
	}
	return result;
}


std::string ConvertIntToText(uint64 in, bool hex, uint8 minCharacters)
{
	std::stringstream result;

	if (hex)
		result << std::setbase(16);

	if (minCharacters != 0)
		result << std::setw(minCharacters);
		
	result << in;

	return result.str();
}


std::string ConvertBinaryArrayToHexText(uint8 const in[], uint16 bytes, bool printLeadingZeros, char fieldSeparator)
{
	uint8 const* const end = in + bytes;
	std::string result;
	bool leadingZerosComplete = false;

	if (bytes == 0)
		return result;

	uint8 const* iptr = in;

	for (; iptr < end; iptr++)
	{
		char msHexDigit = (*iptr) >> 4;
		char lsHexDigit = (*iptr) & 0xF;

		if (leadingZerosComplete || printLeadingZeros || msHexDigit != 0)
		{
			result += WriteHexDigit(uint8(msHexDigit));
			leadingZerosComplete = true;
		}

		if (leadingZerosComplete || printLeadingZeros || lsHexDigit != 0)
		{
			result += WriteHexDigit(uint8(lsHexDigit));
			leadingZerosComplete = true;
		}

		if (fieldSeparator != '\0' && (iptr < end - 1) && leadingZerosComplete) // only put separator between values 
			result += fieldSeparator;
	}

	return result;
}

std::string ConvertBinaryArrayToHexTextBlock(char const title[], uint8 const in[], uint16 bytes, uint16 lineLength, bool printOffset)
{
	std::stringstream result;

	result << title << std::endl;
	result << "================================" << std::endl;
	if (bytes == 0)
		return result.str();

	result << std::hex; 

	uint8 const* iptr = in;
	do
	{
		uint16 bytesToPrint = (bytes < lineLength) ? bytes : lineLength;
		bytes -= bytesToPrint;

		if (printOffset)
			result << std::setfill('0') << std::setw(3) << unsigned(iptr - in)  << "   "; 

		result << ConvertBinaryArrayToHexText(iptr, bytesToPrint, true, ' ') << std::endl;

		iptr += bytesToPrint;
	} while (bytes > 0);

	result << std::endl;
	return result.str();
}

std::string SqlDate(uint year, uint month, uint day)
{
	std::stringstream result;

	result << 
	std::setfill('0') << std::setw(2) << year << '-' << 
	std::setfill('0') << std::setw(2) << month << '-' << 
	std::setfill('0') << std::setw(2) << day;	
	return result.str();
}

std::string SqlTime(uint hour, uint min, uint sec)
{
	std::stringstream result;

	result << 
	std::setfill('0') << std::setw(2) << hour << ':' << 
	std::setfill('0') << std::setw(2) << min << ':' << 
	std::setfill('0') << std::setw(2) << sec;
	
	return result.str();
}

sint8 FindMonth(std::string const& monthString)	// either 3 letter or full name
{
	std::string abbreviation = monthString.substr(0,3);

	if (abbreviation == "Jan") return 1;
	if (abbreviation == "Feb") return 2;
	if (abbreviation == "Mar") return 3;
	if (abbreviation == "Apr") return 4;
	if (abbreviation == "May") return 5;
	if (abbreviation == "Jun") return 6;
	if (abbreviation == "Jul") return 7;
	if (abbreviation == "Aug") return 8;
	if (abbreviation == "Sep") return 9;
	if (abbreviation == "Oct") return 10;
	if (abbreviation == "Nov") return 11;
	if (abbreviation == "Dec") return 12;
	return -1;
}

sint16 ConvertBase64TextToBinaryArray(char const in[], uint8 out[], uint16 outputBytes)
{
	static const unsigned bitsPerCharacter = 6;
	static const unsigned bitsPerByte = 8;
	uint32 store = 0;
	uint8 bits = 0;

	memset(out,0,outputBytes);

	char const* iptr = in;
	uint8* optr = out;

	uint8 const* const end = out + outputBytes;

	for (;; iptr++)
	{
		uint8 readValue = ConvertBase64CharacterToBinary(*iptr);

		if (readValue == uint8(~0))
		{
			if (IsBase64Terminator(iptr))
			{
				if (bits == 0)
					break;
				else if (bits == 2 * bitsPerCharacter)
				{
					if (optr >= end) // about to overflow output
						return -1;
					*(optr++) = uint8(store >> 4);
				}
				else if (bits == 3 * bitsPerCharacter)
				{
					if (optr >= end - 1) // about to overflow output
						return -1;
					store >>= 2;
					*(optr++) = uint8(store >> 8);
					*(optr++) = uint8(store);
				}
				else
					return -1;
				break;
			}
			else
				return -1;
		}
		else
		{
			store = (store << bitsPerCharacter) + readValue;
			bits += bitsPerCharacter;
			if (bits >= bitsPerByte * 3)
			{
				if (optr >= end - 2) // about to overflow output
					return 0;
				*(optr++) = uint8(store >> 16);
				*(optr++) = uint8(store >> 8);
				*(optr++) = uint8(store);
				bits = 0;
				store = 0;
			}
		}
	}
	
	return uint16(optr - out);
}


std::string IPAddressText(uint32 address, uint16 port)
{
	std::stringstream output;

	for (uint8 i = 0; i < sizeof(address); i++)
	{
		output << int(address >> (8 * (sizeof(address) - i - 1)) & 0xFF);

		if (i < sizeof(address) - 1)
			output << '.';	// all but last
	}

	if (port != 0)
		output << ':' << port;

	return output.str();
}


uint32 GetIpAddressAsInt(sockaddr_in const& address)
{
	return ntohl(address.sin_addr.s_addr);
}


bool IsLoopBack(sockaddr_in const& address)
{
	return address.sin_addr.s_addr == htonl(INADDR_LOOPBACK);
}

sockaddr_in ReadIpAddress(char const text[])
{
	sockaddr_in result;
	SetInvalid(result);
	result.sin_family = AF_INET;

	char const* cptr = SkipWhiteSpace(text);

	uint field = 0;
	const uint numberOfIpFields = 4;
	uint16 digit = 0;	//one digit of the dotted decimal IP address
	uint32 integerAddress = 0;

	//lint -e{440}  (Warning -- for clause irregularity: variable 'numberOfIpFields' tested in 2nd expression does not match 'cptr' modified in 3rd)
	for (;field <= numberOfIpFields; cptr++)
	{
		if (!isdigit(*cptr))
		{
			field++;
			integerAddress <<= 8;
			integerAddress |= digit;
			digit = 0;

			if (*cptr == '\0' || *cptr == ':' || isspace(*cptr))
				break;

			if (*cptr != '.' || (field >= numberOfIpFields))
				goto fail;
		}
		else
		{
			digit = digit * 10 + (*cptr - '0');

			if (digit > 0xFF)
				goto fail;
		}
	}

	if (field != numberOfIpFields)
		goto fail;

	if (integerAddress == 0)
		goto fail;

	result.sin_addr.s_addr = htonl(integerAddress);

	if (*cptr == ':') //alternative is '\0' or space
	{
		cptr++;
		uint port = 0;
		for (;isdigit(*cptr);cptr++)
			port = port * 10 + (*cptr - '0');

		result.sin_port = htons(port);
	}
	return result;

fail:
	SetInvalid(result);
	return result;
}

sockaddr_in ReadIpAddressOrHostName(char const text[])
{
	sockaddr_in result = ReadIpAddress(text);

	if (IsValid(result))
		return result;

	std::string hostName = text;

	size_t colonPosition = hostName.find_first_of(':');

	if (FindCharacter(text, '.') == 0)
		return result;	// does not contain a full stop - it isn't a domain name

	long port = 0;
	if (colonPosition != std::string::npos)
	{
		char const* portText = hostName.c_str() + colonPosition + 1;

		port = strtol(portText, 0, 10);	//port is set to zero on error

		if (port > UINT16_MAX)
			port = 0;

		hostName.erase(colonPosition, std::string::npos); // remove port text from hostName
	}

	struct addrinfo hints, *interfaceList;

	memzero(&hints);
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags |= AI_CANONNAME;

	if (getaddrinfo(hostName.c_str(), 0, &hints, &interfaceList) != 0)	//if successful, must always be followed by a call to freeaddrinfo
		return result;

	sockaddr test = *interfaceList->ai_addr;
	memcpy(&result, &test, sizeof(result));	//copy from protocol agnostic format to ip4 format
	result.sin_port = htons(u_short(port));
	freeaddrinfo(interfaceList);	/* interfaceList is no longer needed */
	return result;
}


bool ConvertFixedLengthHexTextToBinaryArray(char const in[], uint8 out[], uint16 outputBytes, bool separatorsAllowed)
{
	memset(out,0,outputBytes);

	char const* iptr = in;
	bool oneDigitFound = false;
	while (isxdigit(*iptr) || (separatorsAllowed && IsSeparator(*iptr)))
	{
		if (isxdigit(*iptr))
			oneDigitFound = true;
		iptr++;
	}

	if (!oneDigitFound)
		return false;

	char const* const end = iptr;	//one past end

	iptr--; // last valid character

	uint8* optr = out + outputBytes - 1;
	unsigned digitsRead = 0;

	uint8 writeValue = 0; //two text characters form 1 byte of binary output.  writeValue holds the 1st byte during the 2nd loop 
	for (;iptr >= in && optr >= out;iptr--)
	{
		uint8 readValue = ReadHexDigit(*iptr);

		if (readValue > 0xF)
		{
			//invalid
			if (!separatorsAllowed || digitsRead == 0)
				return false;
			
			if (IsSeparator(*iptr))
				continue;
			else
				return false;
		}

		if (digitsRead % 2 == 0)
			writeValue = readValue;	//Even
		else
		{
			//Odd
			*optr = uint8(writeValue | (readValue << 4));
			writeValue = 0;
			optr--;
		}

		//these end of loop actions are not done when a separator is detected
		digitsRead++;
	}

	if (writeValue != 0 && optr >= out)
		*optr = writeValue;

	return true;
}

uint16 ConvertVariableLengthHexTextToBinaryArray(char const in[], uint8 out[], uint16 maxOutputBytes, bool separatorsAllowed)	//Binary array is big endian - justified to low index of out
{
	char const* iptr = in;
	uint8* optr = out;

	uint8 const* const end = out + maxOutputBytes;	//one past end

	for (;optr < end; iptr++)
	{
		uint8 readValue1 = ReadHexDigit(*iptr);

		if (readValue1 > 0xF)
		{
			//invalid
			if (!separatorsAllowed)
				break;
			
			if (IsSeparator(*iptr))
				continue;
			else
				break;
		}

		iptr++;
		uint8 readValue2 = ReadHexDigit(*iptr);

		if (readValue2 > 0xF)
			break;

		//iptr is incremented by the for loop
		*optr = (readValue1 << 4) | readValue2;
		optr++;
	}

	return uint16(optr - out);
}



void ReverseArray(uint8 data[], uint16 length)
{
	uint8* lptr = data;
	uint8* hptr = data + length - 1;

	for (;hptr > lptr; lptr++, hptr--)
	{
		uint8 temp = *hptr;
		*hptr = *lptr;
		*lptr = temp;
	}
}


bool ValidUnsignedInteger(char const text[], bool hex, bool separatorsAllowed)
{
	char const* cptr = text;

	if (cptr[1] == 'x' && cptr[0] == '0')
	{
		hex = true;
		cptr += 2; //skip '0x' if present.  'x' tested first as the test is more likely to fail
	}

	for (;;cptr++)
	{
		if (*cptr == '\0' || isspace(*cptr))
			return (text != cptr) ? true : false;

		if (isdigit(*cptr))
			continue;

		if (hex && isxdigit(*cptr))
			continue;

		if (separatorsAllowed && IsSeparator(*cptr))
			continue;

		return false;
	}
}


bool ValidSignedInteger(char const text[])
{
	//skip leading '-' if present
	return ValidUnsignedInteger((text[0] == '-') ? text+1 : text, false, false);
}


std::string WriteSignedIntegerAsFixedPointNumber(sint32 value, sint32 unitValue, bool includeTrailingZeros)
{
	std::stringstream output;

	if (value < 0)
	{
		output << '-';
		value = -value;
	}
	uint32 integerPart = value / unitValue;

	output << integerPart;

	uint32 decimalPart = value - integerPart * unitValue;
	if (includeTrailingZeros || decimalPart > 0)
	{
		output << '.';
		uint32 movingUnitValue = unitValue / 10;	
		// counts down from 1/10th of the unit value to zero in powers or ten (e.g. for unitValue == 1000 (3 decimal places) starts at 100 then 10, 1, 0


		for(;(includeTrailingZeros && movingUnitValue > 0) || (!includeTrailingZeros && decimalPart > 0); movingUnitValue /= 10)
		{
			uint32 currentDigit = ((decimalPart / movingUnitValue));
			output << currentDigit;

			decimalPart -= currentDigit * movingUnitValue;
		}
	}
	return output.str();
}

bool ReadRealNumber(char const text[], double& result)
{
	std::stringstream string(text);

	if (!(string >> result))
		return false;

	return true;
}

bool ValidMask(uint32 mask)
{
	while ((mask & 0x1) == 1)
		mask >>= 1;

	return mask == 0;
}

uint16 ConvertBinaryArrayToBase64Text(uint8 const in[], uint16 inputBytes, char out[], bool pad)
{
	uint8 const* iptr = in;
	uint8 const* const end = in + inputBytes;
	char* optr = out;

	//new bits arrive from the RIGHT
	for (; iptr < end; iptr += 3)
	{
		sint32 bytes = end - iptr;

		if (bytes > 3)
			bytes = 3;

		// avoid data from outside the array getting into store
		uint32 store = uint32(iptr[0]) << 16;
		
		if (bytes > 1)
			store |= uint32(iptr[1]) << 8;
		
		if (bytes > 2)
			store |= iptr[2];

		*(optr++) = ConvertBinaryToBase64Character(uint8(store >> 18));	//1 input byte requires 2 characters
		*(optr++) = ConvertBinaryToBase64Character(uint8(store >> 12));

		if ((bytes > 1) || pad)
			*(optr++) = (bytes > 1) ? ConvertBinaryToBase64Character(uint8(store >> 6)) : '=';

		if ((bytes > 2) || pad)
			*(optr++) = (bytes > 2) ? ConvertBinaryToBase64Character(uint8(store)) : '=';
	}
	*optr = '\0';
	return uint16(optr - out);
}


uint64 ConvertBinaryArrayToUint64(uint8 const in[], uint8 length)
{
	uint64 result = 0;
	uint8 const* iptr = in;
	uint8 const* end = in + length;

	for (; iptr < end; iptr++)
	{
		result <<= 8;
		result |= *iptr;
	}
	return result;
}


unsigned CountBlocks(unsigned in, unsigned blockSize)
{
	unsigned blockSizeMinus1 = blockSize - 1;
	unsigned inRoundDown = in & ~blockSizeMinus1;
	unsigned roundUp = (in & blockSizeMinus1) ? 1 : 0;
	unsigned out = inRoundDown / blockSize + roundUp; 

	return out;
}


void Snooze(uint msTime)
{
#ifdef _MSC_VER
	Sleep(msTime);
#else
	struct timespec time;

	time.tv_sec = msTime / 1000;
	time.tv_nsec = (msTime % 1000) * 1000000;
	nanosleep(&time, 0);
#endif
}

char const* GetEnv(char const name[]) //returns value without quotes
{
	char const* result = getenv(name);

	if (result)
	{
		static std::string text = result;	// needs to be static because a pointer to it is returned

		if (text[0] == '\"' || text[0] == '\'')
			text.erase(0,1);

		unsigned lastChar = text.length()-1;
		if (text[lastChar] == '\"' || text[lastChar] == '\'')
			text.erase(lastChar,1);

		result = text.c_str();
	}
	return result;
}


uint64 GetMsSinceStart()
{
#ifdef _MSC_VER
	return GetTickCount64();
#else
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC , &now);

	return uint64(now.tv_sec) * 1000 + now.tv_nsec / 1000000;
#endif
}

