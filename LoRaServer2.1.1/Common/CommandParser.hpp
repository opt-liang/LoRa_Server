/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef CONSOLE_COMMAND_PARSER_HPP
#define CONSOLE_COMMAND_PARSER_HPP

#include "General.h"
#include "ConfiguredValue.hpp"
#include "Service.hpp"
#include "Eui.hpp"
#include "WordStore.hpp"
#include "InputError.hpp"

#include <string>
#include <vector>

namespace LoRa
{
	class CypherKey;
}

namespace Application
{
	class Element;
}

class CommandParser
{
protected:
	static const uint configurationNameTextWidth = 30;

	WordStore		wordStore;
	std::string		filename;
	unsigned		lineCount;

	CommandParser() : lineCount(0)
	{}

	virtual ~CommandParser() {}

public:
	void Parse(std::istream & infile, std::string const& filename = ""); // returns at end of file
	void Parse(std::string const& line) {Parse(line.c_str());}
	void Parse(char const line[]);

	class InputError : public ::InputError //exception class
	// WARNING exception MUST be caught while CommandParser is still in scope
	{
	//Parent class - no public constructors
	private:
		CommandParser const&	parser;

	protected:
		InputError(bool mySyntaxError, CommandParser const& myParser, std::string const& myExplanation = "") 
			: ::InputError(mySyntaxError, myExplanation), parser(myParser) {}

	public:
		unsigned CurrentWord() const {return parser.wordStore.CurrentWord();}
	};

	class SyntaxError : public InputError
	// WARNING exception MUST be caught while CommandParser is still in scope
	//Thrown when sytax is incorrect
	{
	public:
		SyntaxError(CommandParser const& myParser, char const myExplanation[] = "") 
			: InputError(true, myParser, myExplanation)
		{}

		SyntaxError(CommandParser const& myParser, std::string const& myExplanation) 
			: InputError(true, myParser, myExplanation)
		{}

		SyntaxError(CommandParser const& myParser, std::stringstream const& myExplanation) 
			: InputError(true, myParser, myExplanation.str()) {}
	};

	class ParameterError : public InputError
	// WARNING exception MUST be caught while CommandParser is still in scope
	//Thrown when sytax is correct but a paramter value is incorrect
	{
	public:
		ParameterError(CommandParser const& myParser, char const myExplanation[] = "") 
			: InputError(false, myParser, myExplanation)
		{}

		ParameterError(CommandParser const& myParser, std::string const& myExplanation) 
			: InputError(false, myParser, myExplanation)
		{}

		ParameterError(CommandParser const& myParser, std::stringstream const& myExplanation) 
			: InputError(false, myParser, myExplanation.str()) {}
	};

protected:
	class QuitCommand //exception class
	{
	};

	virtual CommandParser* CreateParser() = 0;
	virtual void ParsePrivate(std::string const& command) = 0;
	
	void ParseLogCommand();
	void ParseLogFileCommand();

	void ParsePingCommand() const;
	void ParseDeleteAllDataCommand();

	void ParseApplicationCommand();
	void ParseApplicationAddCommand();
	void ParseApplicationDeleteCommand();
	void ParseApplicationServerCommand(bool nonNullApp);
	void ParseApplicationListCommand();

	void ParseApplicationServerAddCommand(bool nonNullApp);
	void ParseApplicationServerSetCommand(bool nonNullApp);
	void ParseApplicationServerDeleteCommand(bool nonNullApp);
	void ParseApplicationServerListCommand(bool nonNullApp);

	void ParseNullServerCommand() {ParseApplicationServerCommand(false);}	//server for the null application

	void ParseConnectionCommand();
	void ParseConnectionListCommand();
	void ParseConnectionTestCommand();

	void ParseSetCommand();
	void ParseSetPrivateCommand(std::string const& command);
	void ParseSetListCommand() const;

	void Write(std::string const& text) const;
	void Write(std::stringstream const& text) const {Write(text.str());}
	void Broadcast(std::string const& text) const;
	void Broadcast(std::stringstream const& text) const {Broadcast(text.str());}

	void PrintApplicationList(bool printServers) const;
	std::string PrintApplication(Application::Element const& app, bool printServers) const;

	void EchoLine(char const line[]) const;

	Service::Mask ReadServiceMask(Service::Mask previous = 0);

	void RespondToInputError(bool syntaxError, char const currentLine[], unsigned currentWord, std::string const& text);

	static std::string WriteConfigurationValue(char const name[], bool value) {return WriteConfigurationValue(name, value ? "true" : "false");}
	static std::string WriteConfigurationValue(char const name[], int value, bool hex = false);
	static std::string WriteConfigurationValue(char const name[], unsigned value, bool hex = false);
	static std::string WriteConfigurationValue(char const name[], std::string const& value) {return WriteConfigurationValue(name, value.c_str());}
	static std::string WriteConfigurationValue(char const name[], std::stringstream const& value) {return WriteConfigurationValue(name, value.str());}
	static std::string WriteConfigurationValue(char const name[], char const value[]);

	static std::string WriteConfigurationValue(ConfiguredValueBaseType const& value) {return WriteConfigurationValue(value.Name().c_str(), value.ValueText());}

	EuiType GetExistingApplicationEui(bool thowExceptionOnFail = true);
	//If thowExceptionOnFail is true throws exception if it cannot, otherwise returns invalidEui
	// Only useful when application should exist

private:
	EuiType GetExistingApplicationEuiPrivate();	//only called from GetExistingApplicationEui(bool thowExceptionOnFail)

};

#endif

