/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "General.h"
#include "ExceptionClass.hpp"
#include "GlobalData.hpp"
#include "Thread.hpp"
#include "Utilities.hpp"
#include "DebugMonitor.hpp"
#include "InputError.hpp"
#include "ServerThreadFunctions.hpp"

#include <stdio.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <signal.h>

THREAD_RUN_RETURN_TYPE TimeThreadFunction(void* x = 0);

Thread jsonTcpWaitThread(JsonTcpWaitThreadFunction);
Thread jsonTcpReceiveThread(JsonTcpReceiveThreadFunction);
Thread tcpConnectThread(TcpConnectThreadFunction);
Thread udpReceiveThread(UdpReceiveThreadFunction);

int main(int argc, char const* const argv[])
{
#ifndef _MSC_VER
	signal(SIGPIPE, SIG_IGN);
#endif
	try
	{
		bool useConsole = false;
		std::string logFileName;
		std::string databaseUser;
		std::string databasePassword;

		//scan for switches
		//lint --e{850} (Info -- for loop index variable 'i' whose type category is 'integral' is modified in body of the for loop)
		for (int i = 1; i < argc; i++)
		{
			bool last = i == argc -1;
			if (argv[i][0] == '-')
			{
				std::string parameter = &argv[i][1];

				if (Debug::SetPrintLevel(parameter))
					continue;

				else if (parameter == "console")
					useConsole = true;

				else if (parameter == "noconsole")
					useConsole = false;

				else if (parameter == "port" && !last)
				{
					i++;
					Global::localIpReceivePort = uint16(strtol(argv[i],0,10));
				}

				else if (parameter == "log" && !last)
				{
					i++;
					logFileName = argv[i];
				}

				else if ((parameter == "dbuser") && !last)
				{
					i++;
					databaseUser = argv[i];
				}

				else if ((parameter == "dbpass") && !last)
				{
					i++;
					databasePassword = argv[i];
				}

				else if ((parameter == "dbname") && !last)
				{
					i++;
					Global::databaseName = argv[i];
				}

				else if ((parameter == "dbhost") && !last)
				{
					i++;
					Global::databaseHost = argv[i];
				}

				else
					throw InputError(std::string("Unrecognised parameter ") + parameter);
			}
			else
			{
				//Set log file first
				if (logFileName.empty() && last)
					logFileName = argv[i];
			}
		}

		if (databaseUser.empty())
		{
			std::cerr <<  "LoRa database username must be set" << std::endl;
			exit (-2);
		}

		if (databasePassword.empty())
		{
			std::cout << "Please enter DB password: ";
			std::cin >> databasePassword;
		}

		if (!Global::genericDatabase.Connect(Global::databaseHost, Global::databaseName, databaseUser, databasePassword))
		{
			std::cerr <<  "LoRa " << Global::programDescription << " is unable to connect to its database - fatal error : " << Global::genericDatabase.ErrorNumber() << "  " << Global::genericDatabase.ErrorText() << std::endl;
			exit (-1);
		}

		if (!Global::genericDatabase.UpdateStructure())
		{
			std::cerr <<  "LoRa " << Global::programDescription << " is unable to update its database to correct structure - fatal error : " << Global::genericDatabase.ErrorNumber() << "  " << Global::genericDatabase.ErrorText() << std::endl;
			exit (-3);
		}

		Global::InitialiseData();

		if (Global::localIpReceivePort == 0)
			throw InputError("Local Receive IP port not set");

		Global::tcpConnectionAddressController.SetAdvertisedLocalPort(Global::localIpReceivePort);

		if (!logFileName.empty())
			Debug::SetFile(logFileName);
		
		Debug::SetConsoleDisplay(useConsole);

		if (!Global::OpenUdpSockets())
			throw Exception(Debug::major, __LINE__, __FILE__, 
				std::string("Unable to open LoRa UDP sockets.  ") + IP::lastError.Text(), IP::lastError.Number());

		Global::genericDatabase.SetConfiguredValue("jsonUdpSocket", ConvertIntToText(Global::localIpReceivePort)); //update database to store current port (for web server)

		// Application starts here
		std::cerr << "LoRa " << Global::programDescription << " running" << std::endl;
		sint32 threadResult = 0;

		threadResult = jsonTcpWaitThread.Run();
		if (threadResult != 0)
			throw Exception(__LINE__, __FILE__, "Unable to start TCP wait thread", threadResult);

		threadResult = jsonTcpReceiveThread.Run();
		if (threadResult != 0)
			throw Exception(__LINE__, __FILE__, "Unable to start TCP receive thread", threadResult);

		threadResult = tcpConnectThread.Run();
		if (threadResult != 0)
			throw Exception(__LINE__, __FILE__, "Unable to start TCP connect thread", threadResult);

		threadResult = udpReceiveThread.Run();
		if (threadResult != 0)
			throw Exception(__LINE__, __FILE__, "Unable to start UDP receive thread", threadResult);

		TimeThreadFunction();	// does not return
	}

	catch (InputError const& e)
	{
		std::stringstream text;

		text << e.Text() << std::endl << 
			"Usage : " << argv[0] << " [-console] [-major|-minor|-monitor|-verbose] -port <local UDP port number> -dbuser <db username> -dbpass <db password> [-log <log file name>]" << std::endl << 
			"\"-console\" sets the program to accept input from the console.  Do not use when started with linux \'nohup\' command" << std::endl <<
			"-major|-minor|-monitor|-verbose set the verbosity of the logging" << std::endl;

		Debug::ConsoleWrite(text);
	}

	catch (Exception const& e)
	{
		if (Debug::Print(Debug::major))
		{
			std::stringstream errorText;

			errorText << "File " << e.Filename() << std::dec << " Line " << e.Line() << ": " << e.Explanation();

			if (e.NumberSet())
				errorText << "   Error #" << e.Number();

			Debug::Write(errorText);
		}
	}
	Debug::Write("Program closing");

	/* Release Winsock DLL resources */
#ifdef _MSC_VER
	WSACleanup();
#endif
	return 0;
}


THREAD_RUN_RETURN_TYPE TimeThreadFunction(void*)
{
	for(;;)
	{
		try
		{
			Snooze(Global::timeThreadTickPeriod_ms);
			Global::tcpConnectionManager.Tick();
			Global::ServerSpecificTick();
		}
		catch (Exception const& e)
		{
			if (e.Level() >= Debug::GetPrintLevel())
			{
				std::stringstream errorText;
				
				errorText << "Time thread exception : " << e.Filename() << ":" << e.Line() << ": " << e.Explanation();

				if (e.NumberSet())
					errorText << " " << e.Number();

				Debug::Write(errorText);
				errorText << std::endl;
				Global::commandLineInterface.Write(errorText);
			}
			Snooze(Global::sleepBeforeRestart_ms);
		}
		catch (std::exception const& e)
		{
			std::stringstream errorText;
			errorText << "std::exception " << e.what();

			Debug::Write(errorText);
		}
		catch (...)
		{
			std::stringstream errorText;
			errorText << "TimeThreadFunction Unknown exception caught " << __FILE__;

			Debug::Write(errorText);
		}
	}
}

