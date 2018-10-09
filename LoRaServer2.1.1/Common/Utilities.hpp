/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include "General.h"
#include "Ip.h"

#include <string>
#include <vector>
#include <sstream>

//lint --e{451} (Warning -- Header file math.h repeatedly included but does not have a standard include guard
#include <math.h>
#include <limits.h>
#include <limits>

#ifdef _MSC_VER
	#define strcasecmp(a, b) _stricmp(a, b)	//MS Visual studio does not provide strcasecmp
#else
	#include <strings.h>
#endif

//#define FILE_DEBUG_NEEDED
#if FILE_DEBUG_NEEDED
	#include <fstream>
#endif

void ConvertStringToWordVector(std::string const& in, std::vector<std::string>& out);	//stops on first '#' character
void ConvertStringPointerArrayToWordVector(int elements, char const* in[], std::vector<std::string>& out); // in contains 'elements' elements
std::string FindFirstWord(std::string const& in);
std::string ReplaceFileExtension(std::string const& in, char const newExtension[]);
bool IsBlank(std::string const& in);
bool IsCommentLine(std::string const& in);	//first non black character is '#'
inline uint8 ConvertBase64CharacterToBinary(char in); //a single Base64 char to 6 bit binary, or ~0 on failure
inline char ConvertBinaryToBase64Character(uint8 in); //a single Base64 char to 6 bit binary, or ~0 on failure

inline char const* FindCharacter(char const text[], char const quarry);
inline char const* FindCharacter(char const text[], char const quarryString[]); // finds first char in text that occurs in quarryString
inline char const* FindWhiteSpace(char const text[]);
inline char const* SkipWhiteSpace(char const text[]);
inline uint8 ReadHexDigit(char in); // returns 0xFF if not hex
inline char WriteHexDigit(uint8 in); // only LS 4 bits are used
inline uint32 ReadUnsignedInteger(char const text[], bool hex = false, bool separatorsAllowed = false); //Returns all ones on error
inline uint32 ReadUnsignedInteger(std::string const& text, bool hex = false, bool separatorsAllowed = false) {return ReadUnsignedInteger(text.c_str(), hex, separatorsAllowed);}
inline uint64 ReadUnsignedLongInteger(char const text[], bool hex = false, bool separatorsAllowed = false); //Returns all ones on error
inline uint64 ReadUnsignedLongInteger(std::string const& text, bool hex = false, bool separatorsAllowed = false) {return ReadUnsignedLongInteger(text.c_str(), hex, separatorsAllowed);}
inline sint32 ReadSignedInteger(char const text[], bool hex = false); //Returns INT_MIN on error
inline sint32 ReadSignedInteger(std::string const& text, bool hex = false)  {return ReadSignedInteger(text.c_str(), hex);}
inline sint32 ReadSignedFixPointNumberAsInteger(char const text[], uint8 maxDecimalPlaces);
inline sint32 ReadSignedFixPointNumberAsInteger(std::string const& text, uint8 maxDecimalPlaces)  {return ReadSignedFixPointNumberAsInteger(text.c_str(), maxDecimalPlaces);}
inline uint32 ReadUnsignedFixPointNumberAsInteger(char const text[], uint8 maxDecimalPlaces);
inline uint32 ReadUnsignedFixPointNumberAsInteger(std::string const& text, uint8 maxDecimalPlaces);
inline sint8 ReadBoolean(char const text[]);	//returns 0 or 1 or -1 on error
inline sint8 ReadBoolean(std::string const& text) {return ReadBoolean(text.c_str());}
bool ReadRealNumber(char const text[], double& result);
inline bool ReadRealNumber(std::string const& text, double& result) {return ReadRealNumber(text.c_str(), result);}
std::string WriteSignedIntegerAsFixedPointNumber(sint32 value, sint32 unitValue, bool includeTrailingZeros = false);
bool ValidUnsignedInteger(char const text[], bool hex = false, bool separatorsAllowed = false);
inline bool ValidUnsignedInteger(std::string const& text, bool hex = false, bool separatorsAllowed = false) {return ValidUnsignedInteger(text.c_str(), hex, separatorsAllowed);}
bool ValidSignedInteger(char const text[]);
inline bool ValidSignedInteger(std::string const& text) {return ValidSignedInteger(text.c_str());}
bool ValidMask(uint32 mask); // value equals 2^n-1 where n is any number between 0 and 31

std::string ConvertIntToText(uint64 in, bool hex = false, uint8 minCharacters = 0);
std::string ConvertIntToHexText(uint64 in, uint8 minCharacters = 0, char fieldSeparator = '\0');
inline std::string AddressText(uint32 in, char fieldSeparator = ':') {std::string out = ConvertIntToHexText(uint64(in), 2 * sizeof(in), fieldSeparator); return out;}	//fieldSeparator == '\0' gives no separator
inline std::string AddressText(uint64 in, char fieldSeparator = '-') {std::string out = ConvertIntToHexText(in, 2 * sizeof(in), fieldSeparator); return out;}	//fieldSeparator == '\0' gives no separator

uint32 GetIpAddressAsInt(sockaddr_in const& address);
std::string IPAddressText(uint32 address, uint16 port = 0);
inline std::string AddressText(sockaddr_in const& address) {return IPAddressText(GetIpAddressAsInt(address), ntohs(address.sin_port));}
sockaddr_in ReadIpAddress(char const text[]);	//expects format 129.168.0.1 or 129.168.0.1:12 (12 is the port number)
inline sockaddr_in ReadIpAddress(std::string const& text) {return ReadIpAddress(text.c_str());}
sockaddr_in ReadIpAddressOrHostName(char const text[]);	//expects format 129.168.0.1, 129.168.0.1:12, www.host.com, www.host.com:12 (12 is the port number)
inline sockaddr_in ReadIpAddressOrHostName(std::string const& text) {return ReadIpAddressOrHostName(text.c_str());}
inline bool IsValidIpAddressOrHostName(char const text[]) {return IsValid(ReadIpAddressOrHostName(text));}
inline bool IsValidIpAddressOrHostName(std::string const& text) {return IsValid(ReadIpAddressOrHostName(text));}

std::string ConvertBinaryArrayToHexText(uint8 const in[], uint16 bytes, bool printLeadingZeros, char fieldSeparator = '\0');
std::string ConvertBinaryArrayToHexTextBlock(char const title[], uint8 const in[], uint16 bytes, uint16 lineLength = 8, bool printOffset = true);
inline std::string ConvertBinaryArrayToHexTextBlock(std::string const& title, uint8 const in[], uint16 bytes, uint16 lineLength = 8, bool printOffset = true)
	{return ConvertBinaryArrayToHexTextBlock(title.c_str(), in, bytes, lineLength, printOffset);}
inline std::string ConvertBinaryArrayToHexTextBlock(std::stringstream const& title, uint8 const in[], uint16 bytes, uint16 lineLength = 8, bool printOffset = true)
	{return ConvertBinaryArrayToHexTextBlock(title.str(), in, bytes, lineLength, printOffset);}

std::string SqlDate(uint year, uint month, uint day);	//returns date in text format used by SQL
std::string SqlTime(uint hour, uint min, uint sec);	//returns time in text format used by SQL
sint8 FindMonth(std::string const& monthString);	// either 3 letter or full name.  Returns month (1-12) or -1 on error
inline sint8 FindMonth(char const monthText[])	{std::string monthString = monthText; return FindMonth(monthString);}	// either 3 letter or full name

bool ConvertFixedLengthHexTextToBinaryArray(char const in[], uint8 out[], uint16 outputBytes, bool separatorsAllowed);	//Binary array is big endian - justified to high index of out
uint16 ConvertVariableLengthHexTextToBinaryArray(char const in[], uint8 out[], uint16 maxOutputBytes, bool separatorsAllowed);	//Binary array is big endian - justified to low index of out
void ReverseArray(uint8 data[], uint16 length);	//reverses order of array content
sint16 ConvertBase64TextToBinaryArray(char const in[], uint8 out[], uint16 maxOutputBytes);
//returns number of bytes written or -1 on error
uint16 ConvertBinaryArrayToBase64Text(uint8 const in[], uint16 inputBytes, char out[], bool pad = true); //if pad is true output length is always a multiple of 4, padded by '=' characters
unsigned CountBlocks(unsigned in, unsigned blockSize);	// blockSize must be a power of 2

uint64 ConvertBinaryArrayToUint64(uint8 const in[], uint8 length);

uint64 GetMsSinceStart(void);
void Snooze(uint msTime);

char const* GetEnv(char const name[]); //returns value without quotes
inline bool IsSeparator(char in) {return in == ':' || in == '-';}
inline bool IsIntegerTerminator(char in) {return isspace(in) || in == '\"' || in == '\'' || in == '.' || in == ',';}

inline bool IsBase64Terminator(char const* ptr)
{
	switch (*ptr)
	{
	case '=':
	case '\0':
	case '\"':
		return true;

	default:
		return false;
	}
}

inline bool IsNegative(char const*& cptr) //cptr is a reference to a pointer
//returns true if cptr points to a '-' and increments cptr if it points to a '-' or '+'
{
	bool negative;

	switch(*cptr)
	{
	case '-':
		negative = true;
		cptr++;
		break;

	case '+':
		negative = false;
		cptr++;
		break;

	default:
		negative = false;
		break;
	}
	return negative;
}


inline char const* FindCharacter(char const text[], char const quarry)
{
	char const *ptr = text;

	if (ptr == 0)
		return 0;

	//lint --e{722} (Info -- Suspicious use of ;)
	for (;*ptr != quarry && *ptr != '\0'; ptr++); //search for quarry or end of string

	if (*ptr != '\0')
		return ptr;
	else
		return 0;
}


inline char const* FindCharacter(char const text[], char const quarryString[]) // finds first char in text that occurs in quarryString
{
	char const *ptr = text;

	if (ptr == 0)
		return 0;

	for (;; ptr++)
	{
		if (*ptr == '\0')
			return 0;

		char const *qptr = quarryString;

		for (;; qptr++)
		{
			if (*qptr == '\0')
				break;

			if (*ptr == *qptr)
				return ptr;
		}
	}
}

inline char const* FindWhiteSpace(char const text[])
{
	if (text == 0)
		return 0;

	char const* ptr = text;

	//lint --e{722} (Info -- Suspicious use of ;)
	for (; !isspace(*ptr); ptr++);

	if (*ptr == '\0')
		return 0;

	return ptr;
}


inline char const* SkipWhiteSpace(char const text[])
{
	if (!text)
		return 0;

	char const* ptr = text;

	//lint --e{722} (Info -- Suspicious use of ;)
	for (; isspace(*ptr); ptr++);

	return ptr;
}

inline uint8 ReadHexDigit(char in)
{
	if (in >= '0' && in <= '9')
		return uint8(in - '0');

	in = char(tolower(in));
	if (in >= 'a' && in <= 'f')
		return uint8((in - 'a') + 0xA);

	return 0xFF;
}


inline char WriteHexDigit(uint8 in) // only LS 4 bits are used
{
	char result;

	in &= 0x0F;

	if (in > 9)
		result = char('a' + in - 0xA);
	else
		result = char('0' + in);

	return result;
}


inline uint32 ReadUnsignedInteger(char const text[], bool hex, bool separatorsAllowed) //Returns all ones on error
{
	char const* cptr = text;
	uint32 result = 0;
	unsigned digitsRead = 0;

	if (cptr[1] == 'x' && cptr[0] == '0')
	{
		hex = true;
		cptr += 2; //skip '0x' if present.  'x' tested first as the test is more likely to fail
	}

	for (;*cptr != '\0';digitsRead++, cptr++)
	{
		uint8 readValue = ReadHexDigit(*cptr);

		if (readValue > 0xF)
		{
			if (!separatorsAllowed || digitsRead == 0)
				break;
			
			if (IsSeparator(*cptr))
				continue;
			else if (IsIntegerTerminator(*cptr))
				break;
			else
				return ~uint32(0);
		}

		if (!hex && readValue > 9)
			return ~uint32(0);

		if (hex)
			result <<= 4;
		else
			result *= 10;

		result += readValue;
	}

	if (digitsRead == 0)
		result = ~uint32(0);

	return result;
}


inline uint64 ReadUnsignedLongInteger(char const text[], bool hex, bool separatorsAllowed) //Returns all ones on error
{
	char const* cptr = text;
	uint64 result = 0;
	unsigned digitsRead = 0;

	if (cptr[1] == 'x' && cptr[0] == '0')
	{
		hex = true;
		cptr += 2; //skip '0x' if present.  'x' tested first as the test is more likely to fail
	}

	for (;*cptr != '\0';digitsRead++,cptr++)
	{
		uint8 readValue = ReadHexDigit(*cptr);

		if (readValue > 0xF)
		{
			if (!separatorsAllowed || digitsRead == 0)
				break;

			if (IsSeparator(*cptr))
				continue;
			else if (IsIntegerTerminator(*cptr))
				break;
			else
				return ~uint64(0);
		}

		if (!hex && readValue > 9)
			return ~uint64(0);

		if (hex)
			result <<= 4;
		else
			result *= 10;

		result += readValue;
	}

	if (digitsRead == 0)
		result = ~uint64(0);

	return result;
}

inline sint32 ReadSignedInteger(char const text[], bool hex) //Returns INT_MIN on error
{
	char const* cptr = text;
	bool negative = IsNegative(cptr);

	uint32 temp = ReadUnsignedInteger(cptr, hex);

	if (temp == ~uint32(0))	//all ones
		return SINT32_MIN;

	if (temp > SINT32_MAX)
		return SINT32_MAX;

	return negative ? -signed(temp) : signed(temp);
}

inline sint32 ReadSignedFixPointNumberAsInteger(char const text[], uint8 maxDecimalPlaces)
{
	char const* cptr = text;
	bool negative = IsNegative(cptr);

	uint32 temp = ReadUnsignedFixPointNumberAsInteger(cptr, maxDecimalPlaces);

	if (temp == ~uint32(0))	//all ones
		return SINT32_MIN;

	if (temp > SINT32_MAX)
		return SINT32_MIN;

	return negative ? -sint32(temp) : sint32(temp);
}


inline uint32 ReadUnsignedFixPointNumberAsInteger(char const text[], uint8 maxDecimalPlaces)
{
	char const* cptr = text;
	unsigned result = 0;
	unsigned digitsRead = 0;
	uint8 decimalPlaceCount = 0;
	bool pointFound = false;

	for (; !pointFound || (decimalPlaceCount < maxDecimalPlaces); cptr++)
	{
		if (*cptr != '.')
		{
			uint8 readValue = ReadHexDigit(*cptr);

			if (readValue > 9)
				break;

			if (pointFound)
				decimalPlaceCount++;

			result *= 10;

			result += readValue;
			digitsRead++;
		}
		else
		{
			if (pointFound)
				return ~uint32(0);	// all ones

			pointFound = true;
		}
	}

	if (digitsRead == 0)
		return ~uint32(0);	// all ones

	while (decimalPlaceCount < maxDecimalPlaces)
	{
		decimalPlaceCount++;
		result *= 10;
	}

	return result;
}


inline sint8 ReadBoolean(char const text[])	//returns 0 or 1 or -1 on error
{
	//biased to accept rather than reject
	switch(tolower(text[0]))
	{
	case 't':	return 1;
	case 'f':	return 0;
	case '\0':	return -1;	//watch for end of string

	case 'o':	//on or off
		switch(tolower(text[1]))
		{
		case 'n':	return 1;
		case 'f':	return 0;
		}
	}
	return -1;
}


inline uint8 ConvertBase64CharacterToBinary(char in)
{
	if (in >= 'A' && in <= 'Z')
		return uint8(in - 'A');
	else if (in >= 'a' && in <= 'z')
		return uint8(in - 'a' + 26);
	else if (in >= '0' && in <= '9')
		return uint8(in - '0' + 52);
	else if (in == '+')
		return 62;
	else if (in == '/')
		return 63;
	else
		return ~0;
}

char ConvertBinaryToBase64Character(uint8 in)
{
	in &= 0x3F;

	if (in <= 25)
		return char(in + 'A');
	else if (in <= 51)
		return char(in - 26 + 'a');
	else if (in <= 61)
		return char(in - 52 + '0');
	else if (in == 62)
		return '+';
	else 
		return '/';
}

inline int operator!=(sockaddr_in const& l, sockaddr_in const& r) {return !(l==r);}

inline bool CaseInsensitiveEqual(std::string const& l, std::string const& r)
{
	return (strcasecmp(l.c_str(), r.c_str()) == 0) ? true :false;
}

#endif
