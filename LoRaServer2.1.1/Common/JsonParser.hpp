/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef JSON_PARSER_HPP
#define JSON_PARSER_HPP

#include "General.h"
#include "Utilities.hpp"
#include "ExceptionClass.hpp"

#include <ctype.h>
#include <string>

namespace JSON
{
	std::string ReadValue(char const input[]); //returns unquoted string 
	inline std::string ReadValue(std::string const& input) {return ReadValue(input.c_str());}
	
	class Parser
	{
	private:
		enum State {none, name, value};

		char const*		nextStart;	// pointer to where the parser should start searching on next call (null when end of string found)
		bool			inName;		// nextStart is at the start of a 'name'; if false it is in a value
		bool			inObject;	// nextStart is in the target JSON object
		bool			endOfObject;	//end of object has been reached

	public:
		///When parser is called for the 'top level' JSON obect myText must be null terminated
		Parser(char const myText[], bool enterObject = true)
			: nextStart(myText),
				inName(true),
				inObject(!enterObject),
				endOfObject(false)
		{
		}

		char const* FindName();	// find the first or next name
		char const* FindValue(bool inArray = false);	//find the first or next 

		void Update(Parser const& other, bool inArray = false);	//called when other has parsed a sub object to move this object past the sub object

		static bool Match(char const json[], char const text[]);  //Match ignores '"' in characters in 'l' but treats them as the end of the comparison
		//Match ignores '"' characters in json but treats them as the end of the comparison

	private:
		char const* EndOfParsableString()	//called when the end of the parsable string is detected.  Returns 0
		{
			nextStart = 0;
			endOfObject = true;
			nextStart = 0;
			return 0;
		}

		static char const* SkipObject(char const text[]) {return SkipComponent(text, true);}	//text must point to '{'.  Returns pointer one past matching '}' or 0
		static char const* SkipArray(char const text[]) {return SkipComponent(text, false);}	//text must point to '['.  Returns pointer one past matching ']' or 0
		static char const* SkipComponent(char const text[], bool object);	//returns a pointer 1 past either a sub object or an array.  Call only from SkipObject() or SkipArray()

		static inline char const* SkipWhiteSpaceAndDoubleQuotes(char const text[], bool skipComma = false)
		//returns a pointer to the first character after spaces, double quotes and, optionally, commas.  If none, returns 0
		{
			if (!text)
				return 0;

			char const* ptr = text;

			for (; isspace(*ptr) || (*ptr == '\"') || (skipComma && *ptr == ','); ptr++)
			{
				if (*ptr == '\0')
					return 0;
			}
			return ptr;
		}

		static inline char const* Find1PastColon(char const text[])
		//returns a pointer to the first character the next colon.  If none, returns 0
		{
			if (!text)
				return 0;

			char const* ptr = text;

			for (; *ptr != ':'; ptr++)
			{
				if (*ptr == '\0')
					return 0;
			}
			return ptr + 1;
		}

		static char const* FindFirstCharacterBeyondValue(char const startOfValue[])	
		//WARNING - this function accesses memory one place BELOW startOfValue - but it is okay as the function is private and a value never starts at the begining of a string
		//startOfValue must be beyond the opening quote, return one beyond the closing quote
		{
			if (*startOfValue == '{')
				return SkipObject(startOfValue);

			else if (*startOfValue == '[')
				return SkipArray(startOfValue);

			bool const isQuoted = (*(startOfValue-1) == '\"') ? true : false;
			char const* ptr = startOfValue;

			if (isQuoted)
			{
				for (; *ptr != '\"' && *ptr != '\0'; ptr++)
					;

				if (*ptr == '\"')
					ptr++;
			}
			else
			{
				//if unquoted simple text - a number or 'true' or 'false'
				while(!isspace(*ptr) && *ptr != ','  && *ptr != '}' && *ptr != ']'&& *ptr != '\0')
					ptr++;
			}

			ptr = SkipWhiteSpace(ptr);

			if (*ptr == '\0')
				ptr = 0;

			return ptr;
		}
	};
}

#endif

