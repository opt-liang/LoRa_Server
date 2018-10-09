/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

// Defines the entry point for the console application.
//

#include "General.h"
#include "Thread.hpp"
#include "Utilities.hpp"
#include "UdpSocket.hpp"
#include "CommandLineInterface.hpp"
#include "JsonString.hpp"
#include "JsonCommand.hpp"
#include "JsonException.hpp"
#include "JsonParser.hpp"
#include "InputError.hpp"
#include "LoRaIpPorts.hpp"
#include "PendingAcknowledgeQueue.hpp"
#include "ServerAddressControllerCC.hpp"
#include "WordStore.hpp"

#include <iostream>
#include <stdlib.h>	//for strtol in Linux

namespace JSON
{
	namespace Receive
	{
		void Top(char const text[], sockaddr_in const& source);
		void Command(char const text[], sockaddr_in const& source);
		void Acknowledgement(char const text[]);
	}
}

static const uint16 timeThreadSleepTime_ms = 100;
static const uint16 commandAcknowedgeTime_ms = 3000;

THREAD_RUN_RETURN_TYPE ConsoleThreadFunction(void* x = 0);
THREAD_RUN_RETURN_TYPE TimeThreadFunction(void* x = 0);
void CommandQueueActionFunction(sockaddr_in const& serverAddress, std::string const& command);

Thread consoleThread(ConsoleThreadFunction);
Thread timeThread(TimeThreadFunction);
UDP::Socket serverSocket;


uint16 commandSequenceNumber;


ServerAddressControllerCC addressController;

void ReadLine(char const line[]);
void ParseIncludeCommand(WordStore& wordStore);

PendingAcknowledgeQueue commandQueue(CommandQueueActionFunction, commandAcknowedgeTime_ms / timeThreadSleepTime_ms);

void PrintHelp(char const programName[]);

int main(int argc, char* argv[])
{
	Debug::SetPrintLevel(Debug::major);
	try
	{
		// read app and net server addresses if they are there
		for (int i = 1; i < argc; i++)
		{
			bool last = i == argc -1;

			if (argv[i][0] != '-')
				throw InputError("Parameters must begin with \'-\'");

			std::string parameter = &argv[i][1];

			if (parameter == "h")
				PrintHelp(argv[0]);

			ServerAddressControllerCC::AddressType type = ServerAddressControllerCC::Read(parameter);

			if (type == ServerAddressControllerCC::unknown && !last)
				throw InputError("Unrecognised parameter");

			if (type != ServerAddressControllerCC::unknown && last)
				throw InputError("Parameters -as -ns -cs -nc must be followed by an IP address");

			if (!last)
			{
				i++;

				sockaddr_in serverAddress = ReadIpAddressOrHostName(argv[i]);

				if (!IsValid(serverAddress))
					throw InputError("Unable to read server address");

				if (type == ServerAddressControllerCC::networkServer && !IsValidPort(serverAddress))
					serverAddress.sin_port = htons(LoRa::UDP::jsonPort);

				if (!IsValidPort(serverAddress))
					throw InputError("Unable to read server port address");

				if (!addressController.SetAddress(type, serverAddress))
					throw InputError("Unable to set server port address");
			}
			else
			{
				sockaddr_in serverAddress = ReadIpAddressOrHostName(argv[i]);

				if (!addressController.SetAddress(ServerAddressControllerCC::networkServer, serverAddress))
					throw InputError("Unable to read network server address");
			}
		}
	}

	catch (InputError const& e)
	{
		std::stringstream text;
		
		text << e.Text() << std::endl << std::endl;
		Debug::ConsoleWrite(text);
		PrintHelp(argv[0]);
	}

	//if Network server address has not already been set, set to loopback with the default ports
	if (!addressController.Valid(ServerAddressControllerCC::networkServer))
	{
		sockaddr_in loopback;
		SetInvalid(loopback);

		loopback.sin_family = AF_INET;
		loopback.sin_port = htons(LoRa::UDP::jsonPort);
		loopback.sin_addr.s_addr= htonl(INADDR_LOOPBACK);

		addressController.SetAddress(ServerAddressControllerCC::networkServer, loopback);
	}

	serverSocket.Open(0);

	consoleThread.Run();
	timeThread.Run();

	//Receive and display rx data
	for (;;)
	{
		sockaddr_in source;
		uint8 receivedData[CommandLineInterface::maxReceivedBytes];
		int bytesReceived = serverSocket.Receive(source, receivedData, CommandLineInterface::maxReceivedBytes);

		if (bytesReceived <= 0)
		{
			if (bytesReceived < 0)
			{
				std::stringstream text;
				text << "Unable to receive from socket Error number " << IP::lastError.Text() << std::endl;
				Debug::ConsoleWrite(text);
				Snooze(1000);
			}
			continue;
		}
		receivedData[bytesReceived] = '\0'; // add a NULL to the end 

		JSON::Receive::Top((char const*) receivedData, source);
	}

	return 0;
}


THREAD_RUN_RETURN_TYPE ConsoleThreadFunction(void*)
{
	//wait for console input and send
	for (;;)
	{
		char line[CommandLineInterface::maxReceivedBytes];

		try
		{
			std::cin.getline(line, CommandLineInterface::maxReceivedBytes);

			ReadLine(line);
		}

		catch (...)
		{
			Debug::ConsoleWrite("Unidentified exception thrown\n");
		}
	}
}


THREAD_RUN_RETURN_TYPE TimeThreadFunction(void*)
{
	for (;;)
	{
		Snooze(timeThreadSleepTime_ms);
		commandQueue.Tick();
	}
}

void CommandQueueActionFunction(sockaddr_in const& serverAddress, std::string const& command)
{
	serverSocket.Send(serverAddress, command);
}


//JSON receive functions

void JSON::Receive::Top(char const text[], sockaddr_in const& source)
{
	try
	{
		//JSON format
		JSON::Parser parser(text, true);
		//Find location of packet received from mote
		char const* idText;	//pointer to first character of id
		//valueText is pointer to first character of value text

		//lint --e{720} Info -- Boolean test of assignment
		while (idText = parser.FindName())	//find next object name
		{
			char const* valueText = parser.FindValue();	//find next object value

			if (valueText == 0)
				throw JSON::MessageRejectException("Unable to read command value");

			if (JSON::Parser::Match(idText,"command"))
				JSON::Receive::Command(valueText, source);

			else if (JSON::Parser::Match(idText,"ack"))
				JSON::Receive::Acknowledgement(valueText);

			else if (JSON::Parser::Match(idText,"ackreq"))
			{
				std::string acknowledgement = JSON::ReceiveAcknowledgementRequest(valueText);

				if (!acknowledgement.empty())
					serverSocket.Send(source, acknowledgement);
			}
		}
	}
	catch (JSON::MessageRejectException const& e)
	{
		if (Debug::log.Print(Debug::monitor))
		{
			std::stringstream text;

			text << "Message rejected - " << e.Explanation();
					
			if (e.NumberSet())
				text << " 0x" << std::hex << e.Number();

			text << e.Message() << std::endl;

			Debug::Write(text.str());
		}
	}
}


void JSON::Receive::Command(char const text[], sockaddr_in const& source)
{
	std::string payload = ReadValue(text);

	std::cout << payload << std::endl;
}

void JSON::Receive::Acknowledgement(char const text[])
{
	uint32 number = ReadUnsignedInteger(text);

	if (number > UINT16_MAX)
		return;

	commandQueue.Acknowledged(uint16(number));
}

void ReadLine(char const line[])
{
	WordStore wordStore(line);

	if (wordStore.NoWordsRemaining())
		return;	// blank line

	if (wordStore.NextWord() == "include")
	{
		wordStore.GetNextWord(); //include
		ParseIncludeCommand(wordStore);
	}
	else
	{
		ServerAddressControllerCC::AddressType type = ServerAddressControllerCC::Read(wordStore.NextWord());	//test for as,ns, nc or cs - don't consume word
		if (type == ServerAddressControllerCC::unknown)
		{
			//normal command; send to server
			std::string json = JSON::GenerateCommand(line, true, commandSequenceNumber);

			commandQueue.Add(commandSequenceNumber, addressController.GetAddress(), json);
			commandSequenceNumber++;
		}
		else
		{
			//console command
			wordStore.GetNextWord();	//server type
			if (wordStore.WordsRemaining() > 0)
			{
				sockaddr_in serverAddress = ReadIpAddressOrHostName(wordStore.GetNextWord());

				if (!IsValidPort(serverAddress) || !addressController.SetAddress(type, serverAddress))
					Debug::ConsoleWrite("Server address has not been set\n");
			}

			if (!addressController.Select(type))
				Debug::ConsoleWrite("Cannot select server address\n");
		}
	}
}


void ParseIncludeCommand(WordStore& wordStore)
{
	std::string const& filename = wordStore.GetNextWord();
	if (filename.empty())
		Debug::ConsoleWrite("Include filename missing\n");

	std::ifstream infile(filename.c_str());

	if (!infile.is_open())
	{
		std::stringstream text;
		text << "Unable to open file " << filename << std::endl;

		Debug::ConsoleWrite(text);
		return;
	}

	for (;!infile.eof();)
	{
		char line[CommandLineInterface::maxReceivedBytes];

		infile.getline(line, CommandLineInterface::maxReceivedBytes);

		std::cout << line << std::endl;
		ReadLine(line);	//recursive
	}
}


void PrintHelp(char const programName[])
{
	std::stringstream text;
	
	text << "Usage : "<< programName << " [-ns <network server port>] [-as <application server port>] [-cs <customer server port>] [-nc <network controller port>]" << std::endl;

	Debug::ConsoleWrite(text);
}

