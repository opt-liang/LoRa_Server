/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "CommandParserAS.hpp"
#include "GlobalDataAS.hpp"
#include "Application.hpp"
#include "Utilities.hpp"

#include <iostream>
#include <iomanip>

namespace
{
	std::string const spacer = "   ";
}


void CommandParserAS::ParsePrivate(std::string const& command)
{
	if (command == "mote")
		ParseMoteCommand();

	else throw SyntaxError(*this);
}


void CommandParserAS::ParseMoteCommand()
{
	std::string const& command = wordStore.GetNextWord();	//holds the command word

	if (command == "add")
		ParseMoteAddCommand();
	else if (command == "reset")
		ParseMoteResetCommand();
	else if (command == "delete")
		ParseMoteDeleteCommand();
	else if (command == "list")
		ParseMoteListCommand();

	else throw SyntaxError(*this);
}


void CommandParserAS::ParseMoteAddCommand()
{
	EuiType moteEui = wordStore.GetNextEui();
	ValidValueBool provisioned;
	ValidValueEuiType appEui;
	LoRa::CypherKey key; //if ota, this is the app key, if not, it is the encryption (session) key
	ValidValueUint32 networkAddress;

	bool legacy = false;
	bool first = true;	// legacy can only be detected on first loop

	for (;; first = false)
	{
		std::string const& parameter = wordStore.GetNextWord();

		if (parameter.empty())
			break;

		else if (parameter == "app")
			appEui = GetExistingApplicationEui();

		else if (parameter == "netaddr")
			networkAddress = wordStore.GetNextNetworkAddress();

		else if (parameter == "key")
			key = wordStore.GetNextCypherKey();

		//the provisioned test is last because only the first character is tested
		else if (parameter == "ota")
			provisioned = false;

		else if (parameter == "p" || parameter == "provisioned" || parameter == "personalised" || parameter == "personalized")
			provisioned = true;

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

	if (!provisioned.Valid())
		throw SyntaxError(*this, "\'-ota\' or \'-provisioned\' must be specified");

	if (provisioned)
	{
		if (Global::moteList.Exists(moteEui))
			throw ParameterError(*this, "Mote already exists");

		if (!networkAddress.Valid())
			throw SyntaxError(*this, "Network address must be specified");

		if (!key.Valid())
			key = Global::encryptionKeyDefault;

		Global::moteList.CreateMote(moteEui, networkAddress, appEui, key);
	}
	else
	{
		if (!appEui.Valid())
			throw SyntaxError(*this, "Application must be specified");

		//OTA
		if (!Global::applicationDatabase.WriteJoinMote(moteEui, appEui, key))
			throw ParameterError(*this, "Unable to add mote to database");
	}
}


void CommandParserAS::ParseAddMoteLegacyCommand()
{
	EuiType moteEui = wordStore.GetNextEui();

	if (moteEui < LoRa::invalidNetworkAddress && wordStore.WordsRemaining() <= 1)
	{
		//creating provisioned mote
		LoRa::CypherKey encryptionKey = wordStore.GetNextCypherKey(false);

		if (!encryptionKey.Valid())
			encryptionKey = Global::encryptionKeyDefault;

		Global::moteList.CreateMote(moteEui, uint32(moteEui), nullEui, encryptionKey);
	}
	else
	{
		EuiType applicationEui = GetExistingApplicationEui();

		LoRa::CypherKey applicationKey = wordStore.GetNextCypherKey(false);
		if (!applicationKey.Valid())
			throw SyntaxError(*this, "Cannot read application key");

		//add to database not to Global::moteList because the mote is not active
		if (Global::moteList.Exists(moteEui))
			throw ParameterError(*this, "Mote already exists");

		if (!Global::applicationDatabase.WriteJoinMote(moteEui,applicationEui, applicationKey))
			throw ParameterError(*this, "Unable to add mote to database");
	}
}


void CommandParserAS::ParseMoteResetCommand()
{
	EuiType moteEui = wordStore.GetNextEui();

	bool joinMoteExists = Global::applicationDatabase.JoinMoteExists(moteEui);

	Global::applicationDatabase.DeleteAllNoncesBelongingToMote(moteEui);

	Mote* mote = Global::moteList.GetById(moteEui);

	if (!mote)
	{
		if (!joinMoteExists)
			throw ParameterError(*this, "Mote does not exist");

		return;
	}

	if (mote->Provisioned())
	{
		mote->ResetCommand();
		mote->Unlock();
	}
	else
	{
		mote->Unlock();		//unlock before deletion
		Global::moteList.Delete(moteEui);
	}
}


void CommandParserAS::ParseMoteDeleteCommand()
{
	EuiType moteEui = wordStore.GetNextEui();

	bool joinMoteExists = Global::applicationDatabase.JoinMoteExists(moteEui);

	if (joinMoteExists)
		Global::applicationDatabase.DeleteJoinMote(moteEui);

	bool activeMoteDeleted = Global::moteList.Delete(moteEui);

	if (!joinMoteExists && !activeMoteDeleted)
		throw Exception("Mote does not exist");

	Global::applicationDatabase.DeleteAllNoncesBelongingToMote(moteEui);	// delete any nonces
}


void CommandParserAS::ParseMoteListCommand()
{
	LoRa::DatabaseAS::ActiveMoteClient client(Global::applicationDatabase);

	std::string listType = wordStore.GetNextWord();

	bool otaMotes = listType == "join" || listType == "ota";

	if (otaMotes)
		ParseJoinMoteListCommand();
	else
		ParseActiveMoteListCommand();
}


void CommandParserAS::ParseJoinMoteListCommand()
{
	LoRa::DatabaseAS::JoinMoteClient client(Global::applicationDatabase);

	for (;;)
	{
		LoRa::DatabaseAS::JoinMoteRecord fileRecord = client.Read();

		if (fileRecord.moteEui == invalidEui)
			break;

		std::stringstream output;
		output << AddressText(fileRecord.moteEui) << spacer << AddressText(fileRecord.appEui) << spacer << std::string(fileRecord.applicationKey);
		Write(output);
	}
}


void CommandParserAS::ParseActiveMoteListCommand()
{
	for (unsigned index = 0;; index++)
	{
		Mote* mote = Global::moteList.GetByIndex(index);	//mote is always locked

		if (!mote)
			break;

		std::stringstream output;

		output << AddressText(mote->Id()) << spacer << 
			AddressText(mote->ApplicationEui()) << spacer << 
			std::string(mote->SessionKey());

		mote->Unlock();

		Write(output);
	}
}


void CommandParser::ParseSetPrivateCommand(std::string const& command)
{
	if (CaseInsensitiveEqual(command, Global::allowDuplicateMoteNonce.Name()))
		Global::allowDuplicateMoteNonce = wordStore.GetNextBoolean();
	else if (CaseInsensitiveEqual(command, Global::encryptionKeyDefault.Name()))
		Global::encryptionKeyDefault = wordStore.GetNextCypherKey();
	else
		throw SyntaxError(*this);
}

void CommandParser::ParseSetListCommand() const
{
	std::stringstream text;

	text << std::endl <<
	WriteConfigurationValue(Global::allowDuplicateMoteNonce) << std::endl <<
	WriteConfigurationValue(Global::allowRemoteConfiguration) << std::endl <<
	WriteConfigurationValue(Global::autoCreateMotes) << std::endl <<
	WriteConfigurationValue(Global::encryptionKeyDefault) << std::endl;

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

