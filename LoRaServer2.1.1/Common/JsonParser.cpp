/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "JsonParser.hpp"


char const* JSON::Parser::FindName()
{
	if (endOfObject)
		return 0;

	nextStart = SkipWhiteSpaceAndDoubleQuotes(nextStart);

	if (!nextStart || *nextStart == '\0')
		return EndOfParsableString();

	char const* result = nextStart;
	if (inName && inObject)
	{
		nextStart = Find1PastColon(nextStart);
		nextStart = SkipWhiteSpaceAndDoubleQuotes(nextStart);
		inName = false;

		return result;
	}

	switch(*nextStart)
	{
	case '{':
		if (inObject)
			return EndOfParsableString();	//looking for a name, shouldn't find a sub object
		else
		{
			inObject = true;
			nextStart += 1;
			inName = true;
			return FindName();	//recursive
		}
		break;

	case '}':	//found end of object
		endOfObject = true;
		nextStart = SkipWhiteSpaceAndDoubleQuotes(nextStart + 1, true);
		return 0;
		break;

	case '[':	//looking for a name, shouldn't find an array
	case ']':	//or the end of an array
	case ':':	//or the colon, separating name and value
	case ',':	//should have already moved past the comma separating one object's value from the next's name
	default:
		return EndOfParsableString();
	}
}


char const* JSON::Parser::FindValue(bool inArray)
{
	if (endOfObject)
		return 0;

	if (inArray)
		inName = false;

	nextStart = SkipWhiteSpaceAndDoubleQuotes(nextStart);

	if (!nextStart || *nextStart == '\0')
		return EndOfParsableString();

	if (inName || !inObject)
		return EndOfParsableString();

	char const* result = nextStart;
	nextStart = FindFirstCharacterBeyondValue(nextStart);

	if (!nextStart)
	{
		EndOfParsableString();
		return result;
	}

	nextStart = SkipWhiteSpaceAndDoubleQuotes(nextStart);

	if (!inArray)
		inName = true;

	if (!nextStart)
	{
		EndOfParsableString();
		return result;
	}

	switch (*nextStart)
	{
	case ',':
		nextStart = SkipWhiteSpaceAndDoubleQuotes(nextStart+1);
		break;

	case '[':
	case ']':
	case '{':
	case '}':
		nextStart = EndOfParsableString();
	}
	return result;
}

void JSON::Parser::Update(Parser const& other, bool inArray)
{
	nextStart = other.nextStart;
	inName = inArray ? false : true;

	if (!nextStart)
		return;

	nextStart = SkipWhiteSpaceAndDoubleQuotes(nextStart);

	if (!nextStart || *nextStart == '}' || *nextStart == ']')
		nextStart = 0;
}


//Match ignores '"' characters in json but treats them as the end of the comparison
bool JSON::Parser::Match(char const json[], char const text[])
{
	if (*json == '\"')	json++;

	for (;;json++, text++)
	{
		if (*json == *text && *text != '\0') continue;

		if (*text == '\0')
			return *json == '\"' || *json == '\0';			//end of left word

		return false;
	}
}

char const* JSON::Parser::SkipComponent(char const text[], bool object)
{
	uint level = 0;
	char const open = object ? '{' : '[';
	char const close = object ? '}' : ']';

	char const *ptr = text;
	for (;*ptr != '\0'; ptr++)
	{
		if (*ptr == open)
			level++;

		else if (*ptr == close)
		{
			level--;

			if (level == 0)
				return ptr+1;
		}
	}
	return 0;
}


std::string JSON::ReadValue(char const input[]) //returns unquoted string 
{
	char const* end = input;

	//lint --e{722}  (Info -- Suspicious use of ;)
	for (; *end != '\0' && *end != '\"'; end++);	//empty for

	std::string result;
	result.assign(input,uint(end - input));

	return result;
}

