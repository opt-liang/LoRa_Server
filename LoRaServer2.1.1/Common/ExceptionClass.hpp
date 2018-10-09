/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef EXCEPTION_CLASS_HPP
#define EXCEPTION_CLASS_HPP

#include "General.h"
#include "DebugMonitor.hpp"
#include <string>
#include <sstream>
#include <limits.h>

class Exception
{
private:
	static const sint32 noErrorNumber = sint32(-1);
	Debug::Level level;
	unsigned line;
	std::string	filename;
	std::string explanation;
	sint32 number; // noErrorNumber is 'invalid'
public:
	Exception(Debug::Level myLevel, unsigned myLine, char const myFilename[], char const myExplanation[] = "", sint32 myNumber = noErrorNumber)
		: level(myLevel), line(myLine), filename(myFilename), explanation(myExplanation), number(myNumber)
	{}

	Exception(Debug::Level myLevel, unsigned myLine, char const myFilename[], std::string const&myExplanation, sint32 myNumber = noErrorNumber)
		: level(myLevel), line(myLine), filename(myFilename), explanation(myExplanation), number(myNumber)
	{}

	Exception(unsigned myLine, char const myFilename[], char const myExplanation[] = "", sint32 myNumber = noErrorNumber)
		: level(Debug::major), line(myLine), filename(myFilename), explanation(myExplanation), number(myNumber)
	{}

	Exception(unsigned myLine, char const myFilename[], std::stringstream const& myExplanation, sint32 myNumber = noErrorNumber)
		: level(Debug::major), line(myLine), filename(myFilename), explanation(myExplanation.str()), number(myNumber)
	{}

	Exception(unsigned myLine, char const myFilename[], std::string const& myExplanation, sint32 myNumber = noErrorNumber)
		: level(Debug::major), line(myLine), filename(myFilename), explanation(myExplanation), number(myNumber)
	{}

	Exception(char const myExplanation[] = "", sint32 myNumber = noErrorNumber)
		: level(Debug::major), line(0), explanation(myExplanation), number(myNumber)
	{}

	Exception(std::string const& myExplanation, sint32 myNumber = noErrorNumber)
		: level(Debug::major), line(0), explanation(myExplanation), number(myNumber)
	{}

	virtual ~Exception() {}

	Debug::Level Level() const {return level;}
	unsigned Line() const {return line;}
	std::string const& Filename() const {return filename;}
	std::string const& Explanation() const {return explanation;}
	bool NumberSet() const {return number != noErrorNumber;}
	sint32 Number() const {return number;}
};


#endif

