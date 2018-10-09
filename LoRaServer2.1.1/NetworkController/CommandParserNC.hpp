/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef COMMAND_PARSER_CS_HPP
#define COMMAND_PARSER_CS_HPP

#include "CommandParser.hpp"
#include "General.h"

#include <string>
#include <vector>

class CommandParserNC : public CommandParser
{
public:
	CommandParserNC()
	{}

private:
	//redefined virtual functions
	CommandParser* CreateParser() {return new CommandParserNC();}
	void ParsePrivate(std::string const& command);

	void ParseMoteCommand();
	void ParseMoteAddCommand();
	void ParseAddMoteLegacyCommand();	//for backward compatibility
	void ParseMoteDeleteCommand();
	void ParseMoteServiceCommand();
	void ParseMoteResetCommand();
	void ParseMoteListCommand() const;
};

#endif

