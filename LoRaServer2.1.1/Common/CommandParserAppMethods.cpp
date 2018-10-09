/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "CommandParser.hpp"
#include "Utilities.hpp"
#include "ExceptionClass.hpp"
#include "DebugMonitor.hpp"
#include "BuildVersion.hpp"
#include "SqlDb.hpp"
#include "Application.hpp"
#include "GlobalData.hpp"
#include <memory.h>
#include <fstream>
#include <iomanip>


void CommandParser::ParseApplicationCommand()
{
	std::string const& command = wordStore.GetNextWord();	//holds the command word

	if (command == "add")
		ParseApplicationAddCommand();
	else if (command == "delete")
		ParseApplicationDeleteCommand();
	else if (command == "server")
		ParseApplicationServerCommand(true);
	else if (command == "list")
		ParseApplicationListCommand();
	else throw SyntaxError(*this);
}


void CommandParser::ParseApplicationAddCommand()
{
	EuiType eui = wordStore.GetNextEui();

	if (eui == Application::nullAppEui)
		throw ParameterError(*this, "Null Application cannot be added or deleted");

	if (eui == invalidEui)
		throw ParameterError(*this, "Invalid EUI cannot be used");

	if (Global::applicationList.Exists(eui))
		throw ParameterError(*this, "Application EUI is already in use");

	std::string const& name = wordStore.GetNextWord();

	if (name.empty()) 
		throw SyntaxError(*this, "Application name is required");

	std::string const& owner = wordStore.GetNextWord();

	if (Global::applicationList.Exists(name, owner))
		throw ParameterError(*this, "Application name is already in use");

	if (!Global::applicationList.Add(eui, name, owner))
		throw ParameterError(*this,"Unable to create application");
}

void CommandParser::ParseApplicationDeleteCommand()
{
	EuiType eui = GetExistingApplicationEui();

	if (eui == Application::nullAppEui)
		throw ParameterError(*this, "Null Application cannot be added or deleted");

	if (eui == invalidEui)
		throw ParameterError(*this, "Invalid EUI cannot be used");

	if (!Global::applicationList.DeleteById(eui))
		throw ParameterError(*this);
}


void CommandParser::ParseApplicationServerCommand(bool nonNullApp)
{
	std::string const& firstWord = wordStore.GetNextWord();

	if (firstWord == "add")
		ParseApplicationServerAddCommand(nonNullApp);
	else if (firstWord == "set")
		ParseApplicationServerSetCommand(nonNullApp);
	else if (firstWord == "delete")
		ParseApplicationServerDeleteCommand(nonNullApp);
	else if (firstWord == "list")
		ParseApplicationServerListCommand(nonNullApp);
	else
		throw SyntaxError(*this);
}

void CommandParser::ParseApplicationServerAddCommand(bool nonNullApp)
{
	EuiType eui = nonNullApp ? GetExistingApplicationEui() : Application::nullAppEui;

	sockaddr_in address = wordStore.GetNextIpPortAddress();

	std::string const& activeText = wordStore.GetNextWord();

	bool active;
	if (activeText == "active")
		active = true;
	else if (activeText == "passive")
		active = false;
	else
		throw SyntaxError(*this, "Unable to read server active/passive parameter");

	Service::Mask newMask = ReadServiceMask(0);

	if (!Global::applicationList.Exists(eui))
		throw ParameterError(*this);

	Global::applicationList.AddServer(eui, address, active, newMask);
}


void CommandParser::ParseApplicationServerSetCommand(bool nonNullApp)
{
	EuiType eui = nonNullApp ? GetExistingApplicationEui() : Application::nullAppEui;

	sockaddr_in address = wordStore.GetNextIpPortAddress();

	Application::Element* application;

	application = Global::applicationList.GetById(eui);

	if (!application)
		throw ParameterError(*this, "Application does not exist");

	Service::Mask oldMask = application->GetServiceMask(address);
	Service::Mask newMask = ReadServiceMask(oldMask);	//returns Service::errorMask if the input text is not understood

	bool maskSet = false;
	if (newMask != Service::errorMask)
		maskSet = application->SetServiceMask(address, newMask);

	application->Unlock();

	if (!maskSet)
	{
		if (newMask == Service::errorMask)
			throw SyntaxError(*this,"Invalid service value");
		else
			throw ParameterError(*this, "Server is not configured");
	}
}


void CommandParser::ParseApplicationServerDeleteCommand(bool nonNullApp)
{
	EuiType eui = nonNullApp ? GetExistingApplicationEui() : Application::nullAppEui;

	sockaddr_in address = wordStore.GetNextIpPortAddress();

	bool success = Global::applicationList.DeleteServer(eui, address);

	if (!success)
		throw ParameterError(*this);
}


void CommandParser::ParseApplicationServerListCommand(bool nonNullApp)
{
	EuiType eui = nonNullApp ? GetExistingApplicationEui() : Application::nullAppEui;

	Application::Element const* application = Global::applicationList.GetById(eui);		//application is always locked

	if (!application)
		throw ParameterError(*this, "Unable to find Application");

	std::stringstream output;
	application->PrintServerList(output);

	application->Unlock();

	Write(output);
}


void CommandParser::ParseApplicationListCommand()
{
	std::string const& word = wordStore.GetNextWord();

	bool printServers = false;
	if (word == "full")
		printServers = true;

	PrintApplicationList(printServers);
}


void CommandParser::PrintApplicationList(bool printServers) const
{
	Application::Element* application;
	std::string output;
	for (unsigned index = 0;; index++)
	{
		application = Global::applicationList.GetByIndex(index);		//application is always locked

		if (!application)
			break;

		output = PrintApplication(*application, printServers);
		application->Unlock();

		Write(output);
	}
}

std::string CommandParser::PrintApplication(Application::Element const& application, bool printServers) const
{
	std::stringstream output;

	output << std::left << 
		AddressText(application.Id()) << Application::spacer <<
		std::setw(Application::nameWidth) << application.Name() <<
		application.Owner();

	if (printServers)
	{
		output << std::endl << "-----------------------" << std::endl;
		application.PrintServerList(output);
		output << std::endl;
	}
	return output.str();
}



Service::Mask CommandParser::ReadServiceMask(Service::Mask previous)
{
	Service::Mask result = previous;

	for (;;)
	{
		std::string text = wordStore.GetNextWord(); // don't use reference, because text will be altered

		if (text == "")
			break;
		
		bool add = true;
		bool sign = false;
		if (text[0] == '-')
		{
			add = false;
			sign = true;
		}
		else if (text[0] == '+')
		{
			add = true;
			sign = true;
		}
		if (sign)
			text.erase(0, 1);	//remove first character

		Service::Mask change;

		for (change = 1; change <= Service::maxMask; change <<= 1)
		{
			if (text == Service::Text(change))
				break;
		}

		if (change > Service::maxMask)
			return Service::errorMask;

		Service::SetMaskBits(result, add, change);
	}
	return result;
}


EuiType CommandParser::GetExistingApplicationEui(bool thowExceptionOnFail)
{
	try
	{
		return GetExistingApplicationEuiPrivate();
	}
	catch (CommandParser::InputError e)
	{
		if (thowExceptionOnFail)
			throw;

		return invalidEui;
	}
}

EuiType CommandParser::GetExistingApplicationEuiPrivate()
{
	/*input might be one of 
	<eui> [<next word>]
	<name> [<next word>]
	<name> <owner> [<next word>]
	*/

	std::string const& firstWord = wordStore.GetNextWord();

	if (firstWord.empty())
		throw SyntaxError(*this, "Application name is missing");

	EuiType eui = ReadUnsignedLongInteger(firstWord, true, true);

	bool exists;
	if (eui == invalidEui)
	{
		std::string const& name = firstWord;

		if (name == Application::nullAppName)
			throw SyntaxError(*this, "Null application name cannot be used");

		std::string owner = wordStore.GetNextWord();

		eui = Global::applicationList.GetEui(name, owner);

		if (eui == invalidEui)
		{
			if (!owner.empty())
			{
				wordStore.ReturnWord();
				owner.clear();

				eui = Global::applicationList.GetEui(name, "");	//try without the owner
			}
		}
		exists = eui != invalidEui;
	}
	else
		exists = Global::applicationList.Exists(eui);

	if (!exists)
		throw SyntaxError(*this, "Application does not exist");

	return eui;
}

