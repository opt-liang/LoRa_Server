/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "CommandParserCS.hpp"
#include "Eui.hpp"
#include "GlobalDataCS.hpp"
#include "Utilities.hpp"
#include "Application.hpp"
#include "JsonGenerate.hpp"
#include "MoteCS.hpp"

#include <iostream>
#include <iomanip>

namespace
{
	std::string const spacer = "   ";
}


void CommandParserCS::ParsePrivate(std::string const& command)
{
	if (command == "mote")
		ParseMoteCommand();
	else if (command == "sqloutput")
		ParseSetSqlOutput();
	else if (command == "fileoutput")
		ParseSetFileOutput();

	else throw SyntaxError(*this);
}


void CommandParserCS::ParseMoteCommand()
{
	std::string const& command = wordStore.GetNextWord();	//holds the command word

	if (command == "add")
		ParseMoteAddCommand();
	else if (command == "delete")
		ParseMoteDeleteCommand();
	else if (command == "send")
		ParseMoteSendCommand();
	else if (command == "list")
		ParseMoteListCommand();

	else throw SyntaxError(*this);
}


void CommandParserCS::ParseMoteAddCommand()
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


void CommandParserCS::ParseAddMoteLegacyCommand()
{
	EuiType moteEui = wordStore.GetNextEui();
	EuiType appEui = GetExistingApplicationEui();

	if (Global::moteList.Exists(moteEui))
		throw ParameterError(*this, "Mote already exists");

	Global::moteList.CreateMote(moteEui, appEui);
}


void CommandParserCS::ParseMoteDeleteCommand()
{
	EuiType moteEui = wordStore.GetNextEui();
	
	if (!Global::moteList.DeleteById(moteEui))
		throw SyntaxError(*this, "Mote does not exist");
}


void CommandParserCS::ParseMoteSendCommand()
{
	EuiType moteEui = wordStore.GetNextEui();

	ValidValueUint16 motePortNumber;
	std::string dataText;

	for (; wordStore.WordsRemaining();)
	{
		std::string const& word = wordStore.GetNextWord();

		if (word == "port")
			motePortNumber = wordStore.GetNextUnsigned();

		else if (word == "data")
			dataText = wordStore.ReadHexText();

		else throw SyntaxError(*this);
	}

	if (!motePortNumber.Valid() || dataText.length() < 2)
		throw SyntaxError(*this, "Unable to read all parameters");

	if (motePortNumber > 0xFF)
		throw SyntaxError(*this, "Mote port number too big");

	uint8 frameData[LoRa::maxDataBytes + 1];
	uint16 frameDataBytes = ConvertVariableLengthHexTextToBinaryArray(dataText.c_str(), frameData, LoRa::maxDataBytes + 1, false);

	if (frameDataBytes == 0)
		throw SyntaxError(*this, "Unable to interpret frame data text");

	if (frameDataBytes > LoRa::maxDownstreamDataBytes)
		throw SyntaxError(*this, "Frame data text too long");

	LoRa::FrameApplicationData payload;
	
	payload.SetData(frameData, frameDataBytes);
	payload.Port(uint8(motePortNumber));

	if (!Global::moteList.SendAppData(moteEui, payload))
		throw ParameterError(*this, "Mote is unknown");
}


void CommandParserCS::ParseMoteListCommand()
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


void CommandParser::ParseSetPrivateCommand(std::string const& /*command*/)
{
	throw SyntaxError(*this);
}

void CommandParser::ParseSetListCommand() const
{
	std::stringstream text;

	text << std::endl <<
	//autoCreateMotes is not printed because it is not used in the CS, although it exists
	WriteConfigurationValue(Global::allowRemoteConfiguration) << std::endl;

	Write(text.str());
}


void CommandParserCS::ParseSetSqlOutput()
{
	Global::applicationDataOutput.EnableDatabaseWrite(wordStore.GetNextBoolean());
}

void CommandParserCS::ParseSetFileOutput()
{
	std::string const& firstWord = wordStore.GetNextWord();

	bool on;
	if (WordStore::IsBooleanTrue(firstWord))
		on = true;
	else if (WordStore::IsBooleanFalse(firstWord))
		on = false;
	else
	{
		if (firstWord.empty())
			throw SyntaxError(*this);

		on = true;
		wordStore.ReturnWord();	//assume the word is the file name
	}

	if (on)
	{
		std::string const& name = wordStore.GetNextWord();

		if (!name.empty())
		{
			if (!Global::applicationDataOutput.SetOutputFile(name))
				throw SyntaxError(*this);
		}

		if (!Global::applicationDataOutput.FileNameValid())
			throw SyntaxError(*this, "Attempt to open output file without specifying name");
	}

	if (!Global::applicationDataOutput.EnableFileWrite(on))
		throw SyntaxError(*this, "Unable to open output file");
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

