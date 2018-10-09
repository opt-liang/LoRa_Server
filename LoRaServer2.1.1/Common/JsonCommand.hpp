/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef JSON_COMMAND_HPP
#define JSON_COMMAND_HPP

#include "General.h"

#include <string>
#include <sstream>


namespace JSON
{
	//sequenceNumber is only used when requestAcknowledge is true
	std::string GenerateCommand(char const text[], bool requestAcknowledge, uint16 sequenceNumber = 0);
	inline std::string GenerateCommand(std::string const& input, bool requestAcknowledge, uint16 sequenceNumber = 0) {return GenerateCommand(input.c_str(), requestAcknowledge, sequenceNumber);}
	inline std::string GenerateCommand(std::stringstream const& input, bool requestAcknowledge, uint16 sequenceNumber = 0) {return GenerateCommand(input.str(), requestAcknowledge, sequenceNumber);}

	std::string ReceiveAcknowledgementRequest(char const receivedText[]);	// returns acknowledgement string
	std::string GenerateAcknowledgementMessage(uint16 number);
}

#endif
