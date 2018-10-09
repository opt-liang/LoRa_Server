/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "CommandParser.hpp"
#include "GlobalData.hpp"
#include "Utilities.hpp"
#include "ExceptionClass.hpp"
#include "DebugMonitor.hpp"
#include "BuildVersion.hpp"
#include "LoRa.hpp"
#include <memory.h>
#include <fstream>
#include <iomanip>

void CommandParser::Parse(std::istream & infile, std::string const& myfilename)
{
	filename = myfilename;
	bool quit = false;
	
	for (lineCount = 1; !quit && !infile.eof(); lineCount++)
	{
		CommandParser* parser = CreateParser();
		try
		{
			std::string line;

			getline(infile, line);

			CommandParser* parser = CreateParser();
			parser->Parse(line);
		}
		catch (QuitCommand const&)
		{
			quit = true;
		}
		delete parser;
	}
}


void CommandParser::Parse(char const line[])
{
	EchoLine(line);
	if (IsBlank(line) || IsCommentLine(line))
		return;

	wordStore.Read(line);

	if (wordStore.NoWordsRemaining())
		return;

	try
	{
		std::string const& command = wordStore.GetNextWord();	//holds the command word

		if (command == "log")
			ParseLogCommand();
		else if (command == "ping")
			ParsePingCommand();
		else if (command == "deleteAllData")
			ParseDeleteAllDataCommand();
		else if (command == "app" || command == "application")
			ParseApplicationCommand();
		else if (command == "server")
			ParseNullServerCommand();
		else if (command == "connection")
			ParseConnectionCommand();
		else if (command == "set")
			ParseSetCommand();
		else
			ParsePrivate(command);	//virtual function
	}
	catch (WordStore::SyntaxError const& error)
	{
		RespondToInputError(true, line, error.CurrentWord(), error.Text());
	}
	catch (CommandParser::InputError const&(error))
	{
		RespondToInputError(error.SyntaxError(), line, error.CurrentWord(), error.Text());
	}

	catch (SqlDb::Exception const& e)
	{
		std::stringstream out;
		out << "Unable to access to database - any requested change may not have occurred " << e.Filename() << " " << e.Line() << " " << e.Explanation() << std::endl;
		Broadcast(out);
	}
}


void CommandParser::ParseLogCommand()
{
	std::string const& firstWord = wordStore.GetNextWord();

	if (Debug::SetPrintLevel(firstWord))
		return;

	else if (firstWord == "file")
		ParseLogFileCommand();

	else if (firstWord == "display" || firstWord == "console")
	{
		bool consoleDisplay = wordStore.GetNextBoolean(true);
		Debug::SetConsoleDisplay(consoleDisplay);
	}
	else if (firstWord == "help")
	{
		std::stringstream out;
		out << "print \'major|minor|monitor|verbose\'" << std::endl;
		out << "print \'console on|off\'" << std::endl;

		Write(out);
	}
	else throw SyntaxError(*this);
}


void CommandParser::ParseLogFileCommand()
{
	std::string const& name = wordStore.GetNextWord();

	if (name.empty())
		throw SyntaxError(*this, "Unable to read log file name");

	Debug::SetFile(name);
}

void CommandParser::ParsePingCommand() const
{
	std::stringstream out;
	out << Global::programDescription << " is alive (" << BuildVersion::VersionString() << ')';

	Write(out);
}

void CommandParser::ParseDeleteAllDataCommand()
{
	std::string const& confirm = wordStore.GetNextWord();

	if (confirm != "yesREALLY")
		throw SyntaxError(*this);

	Global::DeleteAllData();
}



std::string CommandParser::WriteConfigurationValue(char const name[], int value, bool hex)
{
	std::stringstream textValue;

	if (hex)
		textValue << std::hex;

	textValue << value;

	return WriteConfigurationValue(name, textValue);
}

std::string CommandParser::WriteConfigurationValue(char const name[], unsigned value, bool hex)
{
	std::stringstream textValue;

	if (hex)
		textValue << std::hex;

	textValue << value;

	return WriteConfigurationValue(name, textValue);
}

std::string CommandParser::WriteConfigurationValue(char const name[], char const value[])
{
	std::stringstream result;

	result << std::setw(configurationNameTextWidth) << std::left <<
		name << std::setw(0) << value;

	return result.str();
}


void CommandParser::ParseConnectionCommand()
{
	std::string const& command = wordStore.GetNextWord();
	 
	if (command == "list")
		ParseConnectionListCommand();
	else if (command == "test")
		ParseConnectionTestCommand();
	else
		throw SyntaxError(*this);
}


void CommandParser::RespondToInputError(bool syntaxError, char const currentLine[], unsigned currentWord, std::string const& text)
{
		std::stringstream out;

		out << (syntaxError ? "Syntax" : "Parameter") << " error in command " << "\'" << currentLine << "\'";

		if (filename != "")
			out << "  File :" << filename << " Line :" << lineCount;

		out << " Word " << currentWord;

		if (!text.empty())
			out << "  " << text;

		Write(out);
}


