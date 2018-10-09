/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "CommandParserNC.hpp"
#include "GlobalDataNC.hpp"
#include "Application.hpp"
#include "Utilities.hpp"

#include <iostream>
#include <iomanip>

namespace
{
	std::string const spacer = "   ";
}


void CommandParserNC::ParsePrivate(std::string const& command)
{
	if (command == "mote")
		ParseMoteCommand();

	else throw SyntaxError(*this);
}


void CommandParserNC::ParseMoteCommand()
{
	std::string const& command = wordStore.GetNextWord();	//holds the command word

	if (command == "add")
		ParseMoteAddCommand();
	else if (command == "delete")
		ParseMoteDeleteCommand();
	else if (command == "reset")
		ParseMoteResetCommand();
	else if (command == "service")
		ParseMoteServiceCommand();
	else if (command == "list")
		ParseMoteListCommand();

	else throw SyntaxError(*this);
}


void CommandParserNC::ParseMoteAddCommand()
{
	EuiType moteEui = wordStore.GetNextEui();

	if (Global::moteList.Exists(moteEui))
		throw ParameterError(*this, "Mote already exists");

	ValidValueEuiType appEui;
	bool legacy = false;
	bool first = true;	// legacy can only be detected on first loop

	for (;; first = false)
	{
		std::string const& parameter = wordStore.GetNextWord();

		if (parameter.empty())
			break;

		else if (parameter == "app")
			appEui = GetExistingApplicationEui();


		//does not comply to current syntax - may be legacy
		else if (first)
		{
			legacy = true;
			break;
		}
		else
			throw SyntaxError(*this);
	}

	if (legacy)
	{
		wordStore.ReturnWord(2);	//two words have been read - moteEUI and one other
		ParseAddMoteLegacyCommand();
		return;
	}

	if (!appEui.Valid())
		throw SyntaxError(*this, "Application must be specified");

	Global::moteList.CreateMote(moteEui, appEui);
}


void CommandParserNC::ParseAddMoteLegacyCommand()
{
	EuiType moteEui = wordStore.GetNextEui();
	EuiType appEui = GetExistingApplicationEui();

	if (Global::moteList.Exists(moteEui))
		throw ParameterError(*this, "Mote already exists");

	Global::moteList.CreateMote(moteEui, appEui);
}


void CommandParserNC::ParseMoteDeleteCommand()
{
	EuiType moteEui = wordStore.GetNextEui();
	
	if (!Global::moteList.DeleteById(moteEui))
		throw SyntaxError(*this, "Mote does not exist");
}


void CommandParserNC::ParseMoteResetCommand()
{
	EuiType moteEui = wordStore.GetNextEui();

	Mote* mote = Global::moteList.GetById(moteEui);

	if (!mote)
		throw SyntaxError(*this, "Mote does not exist");

	mote->Reset();
	mote->Unlock();
}

void CommandParserNC::ParseMoteListCommand() const
{
	for (uint i = 0;; i++)
	{
		Mote* mote = Global::moteList.GetByIndex(i);

		if (!mote)
			break;

		std::stringstream output;

		output << AddressText(mote->Id()) << spacer << AddressText(mote->ApplicationEui());

		mote->Unlock();

		Write(output);
	}
}


void CommandParserNC::ParseMoteServiceCommand()
{
	EuiType eui = wordStore.GetNextEui();

	if (!Global::moteList.ReceiveMoteServiceCommand(eui, wordStore))
		throw SyntaxError(*this);
}


void CommandParser::ParseSetPrivateCommand(std::string const& command)
{
	if (CaseInsensitiveEqual(command, Global::defaultMoteChannelMask.Name()))
		Global::defaultMoteChannelMask = wordStore.GetNextUnsigned(true);	//hex
	else if (CaseInsensitiveEqual(command, Global::defaultMoteChannelMaskControl.Name()))
		Global::defaultMoteChannelMaskControl = wordStore.GetNextUnsigned();
	else if (CaseInsensitiveEqual(command, Global::transmissionsOfUnacknowledgedUplinkFrame.Name()))
		Global::transmissionsOfUnacknowledgedUplinkFrame = wordStore.GetNextUnsigned();
	else
		throw SyntaxError(*this);
}


void CommandParser::ParseSetListCommand() const
{
	std::stringstream text;

	text << std::endl <<
	WriteConfigurationValue(Global::autoCreateMotes) << std::endl <<
	WriteConfigurationValue(Global::allowRemoteConfiguration) << std::endl <<
	WriteConfigurationValue(Global::defaultMoteChannelMask) << std::endl <<
	WriteConfigurationValue(Global::defaultMoteChannelMaskControl) << std::endl <<
	WriteConfigurationValue(Global::transmissionsOfUnacknowledgedUplinkFrame) << std::endl;
	Write(text);
}


void CommandParser::Write(std::string const& text) const
{
	Global::commandLineInterface.Write(text);
}

void CommandParser::Broadcast(std::string const& text) const
{
	Global::commandLineInterface.Broadcast(text);
}

void CommandParser::EchoLine(char const /*line*/[]) const
{
}

