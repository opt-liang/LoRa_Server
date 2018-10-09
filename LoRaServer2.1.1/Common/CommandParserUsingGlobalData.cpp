/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "CommandParser.hpp"
#include "GlobalData.hpp"

void CommandParser::ParseSetCommand()
{
	std::string const& text = wordStore.GetNextWord();

	if (text == "" || text == "list")
		ParseSetListCommand();

	else if (CaseInsensitiveEqual(text,Global::allowRemoteConfiguration.Name()))
		Global::allowRemoteConfiguration = wordStore.GetNextBoolean();
	else if (CaseInsensitiveEqual(text, Global::autoCreateMotes.Name()))
		Global::autoCreateMotes = wordStore.GetNextBoolean();
	else
		ParseSetPrivateCommand(text);
}

