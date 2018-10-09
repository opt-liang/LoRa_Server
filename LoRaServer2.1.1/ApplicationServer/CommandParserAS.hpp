/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef COMMAND_PARSER_AS_HPP
#define COMMAND_PARSER_AS_HPP

#include "CommandParser.hpp"
#include "General.h"

#include <string>
#include <vector>

class CommandParserAS : public CommandParser
{
public:
	CommandParserAS()
	{}

private:
	//redefined virtual functions
	CommandParser* CreateParser() {return new CommandParserAS();}
	void ParsePrivate(std::string const& command);

	void ParseMoteCommand();
	void ParseMoteAddCommand();
	void ParseAddMoteLegacyCommand();
	void ParseMoteResetCommand();
	void ParseMoteDeleteCommand();
	void ParseMoteListCommand();

	void ParseJoinMoteListCommand();
	void ParseActiveMoteListCommand();
};

#endif

