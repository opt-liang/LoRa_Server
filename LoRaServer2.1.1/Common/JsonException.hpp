/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef JSON_EXCEPTION_HPP
#define JSON_EXCEPTION_HPP

#include "General.h"
#include <string>

namespace JSON
{
	class MessageRejectException
	{
		//lint --e{1551} (Warning -- Function may throw exception '...' in destructor 'JSON::MessageRejectException::~MessageRejectException(void)
	public:
		static const uint32 noErrorNumber = uint32(UINT32_MAX);

	private:
		std::string				message;
		std::string				explanation;
		uint32					number; // noErrorNumber represents no number

	public:
		MessageRejectException(char const inputString[], char const myExplanation[] = "", uint32 myNumber = noErrorNumber)
			: message(inputString), explanation(myExplanation), number(myNumber)
		{
		}

		MessageRejectException(char const inputString[], std::string const& myExplanation, uint32 myNumber = noErrorNumber)
			: message(inputString), explanation(myExplanation), number(myNumber)
		{
		}

		std::string const& Explanation() const {return explanation;}
		bool NumberSet() const {return number != noErrorNumber;}
		uint32 Number() const {return number;}
		std::string const& Message() const {return message;}
	};
}

#endif