/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "General.h"
#include "ExceptionClass.hpp"
#include "GlobalDataNS.hpp"
#include "Thread.hpp"
#include "GatewayMessageProtocol.hpp"
#include "Utilities.hpp"
#include "IpSocketSet.hpp"
#include "UdpSocket.hpp"
#include "JsonReceiveNS.hpp"
#include "InputError.hpp"
#include "LoRaIpPorts.hpp"
#include "ServerThreadFunctions.hpp"
#include "Queue.hpp"

#include <stdio.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <signal.h>

THREAD_RUN_RETURN_TYPE UdpReceiveThreadFunctionNS(void* x = 0);
THREAD_RUN_RETURN_TYPE UdpQueueServiceThreadFunctionNS(void* x = 0);
THREAD_RUN_RETURN_TYPE TimeThreadFunction(void* x = 0);
THREAD_RUN_RETURN_TYPE GatewayTransmitThreadFunction(void* x = 0);

Thread jsonTcpWaitThread(JsonTcpWaitThreadFunction);
Thread jsonTcpReceiveThread(JsonTcpReceiveThreadFunction);
Thread tcpConnectThread(TcpConnectThreadFunction);
Thread udpReceiveThread(UdpReceiveThreadFunctionNS);	//receives from UDP socket and writes to queue
Thread udpQueueServiceThread(UdpQueueServiceThreadFunctionNS);	//receives from queue and services message
Thread gatewayTransmitThread(GatewayTransmitThreadFunction);


namespace	//UDP message queue definition
{
	const int receiveBufferLength = Global::maxReceivedIpMessage+1;

	struct UdpReceivedMessage
	{
		uint64			receiveTime_ms;
		uint8			data[receiveBufferLength];
		int				dataLength;
		UDP::Socket*	socket;
		sockaddr_in		source;
	};

	typedef QueueTemplate<UdpReceivedMessage> UdpMessageQueue;
}

uint const udpMessageQueueLimit = 50;
uint64 const maxudpMessageQueueDelay_ms = 250;
UdpMessageQueue		udpMessageQueue(false, udpMessageQueueLimit);

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
					LoRa::UDP::gatewayMessageProtocolPort = uint16(strtol(argv[i],0,10));
					LoRa::UDP::jsonPort = LoRa::UDP::gatewayMessageProtocolPort + 1;
					LoRa::TCP::jsonPort = LoRa::UDP::jsonPort;
				}

				else if (parameter == "port-gwmp" && !last)
				{
					i++;
					LoRa::UDP::gatewayMessageProtocolPort = uint16(strtol(argv[i],0,10));
				}

				else if (parameter == "port-json")
				{
					i++;
					LoRa::UDP::jsonPort = uint16(strtol(argv[i],0,10));
					LoRa::TCP::jsonPort = LoRa::UDP::jsonPort;
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

				else if (parameter == "netid" && !last)
				{
					i++;
					std::stringstream idText;
					idText << argv[i];
					idText << std::hex;

					uint16 id = 0;

					if (!(idText >> id))
						throw (InputError("Unable to read netid"));

					Global::joinController.SetNetworkId(id);
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

		Global::tcpConnectionAddressController.SetAdvertisedLocalPort(LoRa::UDP::jsonPort);

		if (!logFileName.empty())
			Debug::SetFile(logFileName);
		
		Debug::SetConsoleDisplay(useConsole);

		if (!Global::OpenUdpSockets())
			throw Exception(Debug::major, __LINE__, __FILE__, 
				std::string("Unable to open LoRa UDP sockets.  ") + IP::lastError.Text(), IP::lastError.Number());

		if (!Global::joinController.IsInitialised())
			throw InputError("Network ID not set");

		Global::genericDatabase.SetConfiguredValue("jsonUdpSocket", ConvertIntToText(LoRa::UDP::jsonPort)); //update database to store current port (for web server)

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

		threadResult = udpQueueServiceThread.Run();
		if (threadResult != 0)
			throw Exception(__LINE__, __FILE__, "Unable to start UDP service thread", threadResult);

		threadResult = gatewayTransmitThread.Run();
		if (threadResult != 0)
			throw Exception(__LINE__, __FILE__, "Unable to start Gateway transmit thread", threadResult);

		TimeThreadFunction();	// does not return
	}

	catch (InputError const& e)
	{
		std::stringstream text;

		text << e.Text() << std::endl << 
			"Usage : " << argv[0] << " [-console] [-major|-minor|-monitor|-verbose] [-netid <LoRa network id>] -dbuser <db username> -dbpass <db password> [-log <log file name>]" << std::endl << 
			"\"-console\" sets the program to accept input from the console.  Do not use when started with linux \'nohup\' command" << std::endl <<
			"-major|-minor|-monitor|-verbose set the verbosity of the logging" << std::endl <<
			"-netid <id> sets the (hexadecimal) value of the LoRa network id.  This need not be used if the value is configured in the program database" << std::endl;

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


THREAD_RUN_RETURN_TYPE UdpReceiveThreadFunctionNS(void*)
{
	for (;;)
	{
		try
		{
			for (;;)
			{
				UdpReceivedMessage* rxMessage = 0;
				try
				{
					UDP::Socket* socket = (UDP::Socket*) Global::udpSocketSet.Wait();

					rxMessage = new UdpReceivedMessage;

					rxMessage->dataLength = socket->Receive(rxMessage->source, rxMessage->data, receiveBufferLength);

					rxMessage->receiveTime_ms = GetMsSinceStart();
					rxMessage->socket = socket;

					if (rxMessage->dataLength <= 0)
					{
						int receivedLength = rxMessage->dataLength;

						delete rxMessage;
						if (receivedLength < 0)
							throw Exception(Debug::major, __LINE__, __FILE__, IP::lastError.Text(), IP::lastError.Number());
						else
							continue;
					}

					else if (rxMessage->dataLength >= receiveBufferLength)
					{
						delete rxMessage;
						throw Exception(Debug::major, __LINE__, __FILE__, "Overlong message received");
					}
					rxMessage->data[rxMessage->dataLength] = '\0'; // add a NULL to the end 

					udpMessageQueue.Add(*rxMessage);	//consumes rxMessage - does not throw exception
					rxMessage = 0;
				}
				catch (...)
				{
					delete rxMessage;
					throw;
				}
			}

		}
		catch (Exception const& e)
		{
			std::stringstream errorText;

			errorText << "Rx Msg exception : " << e.Filename() << ":" << e.Line() << ": " << e.Explanation();

			if (e.NumberSet())
				errorText << " " << std::dec << e.Number();

			Debug::Write(errorText);
		}
		catch (std::exception const& e)
		{
			std::stringstream errorText;
			errorText << "std::exception " << e.what();
			Debug::Write(errorText);
		}

		catch (IP::Error const &e)
		{
			std::stringstream errorText;

			errorText << "UdpReceiveThreadFunction IP exception caught " <<
				e.Text() << "   " << e.Number();

			Debug::Write(errorText);
		}

		catch (...)
		{
			std::stringstream errorText;
			errorText << "UdpReceiveThreadFunction Unknown exception caught " << __FILE__;

			Debug::Write(errorText);
		}

		Global::CloseUdpSockets();
		Snooze(Global::sleepBeforeRestart_ms);
		if (!Global::OpenUdpSockets())
		{
			std::stringstream errorText;
			errorText << "Unable to reopen UDP sockets " << __FILE__;

			Debug::Write(errorText);
		}
	}
}


THREAD_RUN_RETURN_TYPE UdpQueueServiceThreadFunctionNS(void* x)
{
	for (;;)
	{
		try
		{
			for (;;)
			{
				UdpReceivedMessage* rxMessage = 0;

				try
				{
					rxMessage = udpMessageQueue.Read();

					if (rxMessage->socket == &Global::messageProtocolSocket)
					{
						uint64 now_ms = GetMsSinceStart();

						if (rxMessage->receiveTime_ms + maxudpMessageQueueDelay_ms >= now_ms)	//check it wasn't queued too long
						{
							//lint --e{1788}  (Info -- Variable 'parser' (line 241) (type 'GatewayMessageProtocol::Parser') is referenced only by its constructor or destructor)
							uint64 start = GetMsSinceStart();
							GatewayMessageProtocol::Parser parser(Global::messageProtocolSocket, static_cast<uint8 const*>(rxMessage->data), static_cast<uint16>(rxMessage->dataLength), rxMessage->receiveTime_ms, rxMessage->source);
						}
					}
					else
						JSON::Receive::Top((char const*) rxMessage->data, rxMessage->source, false);

					delete rxMessage;
				}
				catch (...)
				{
					delete rxMessage;
					throw;
				}
			}
		}
		catch (Exception const& e)
		{
			std::stringstream errorText;

			errorText << "Rx Msg exception : " << e.Filename() << ":" << e.Line() << ": " << e.Explanation();

			if (e.NumberSet())
				errorText << " " << std::dec << e.Number();

			Debug::Write(errorText);
		}
		catch (std::exception const& e)
		{
			std::stringstream errorText;
			errorText << "std::exception " << e.what();
			Debug::Write(errorText);
		}

		catch (IP::Error const &e)
		{
			std::stringstream errorText;

			errorText << "UdpReceiveThreadFunction IP exception caught " <<
				e.Text() << "   " << e.Number();

			Debug::Write(errorText);
		}

		catch (...)
		{
			std::stringstream errorText;
			errorText << "UdpReceiveThreadFunction Unknown exception caught " << __FILE__;

			Debug::Write(errorText);
		}

		Global::CloseUdpSockets();
		Snooze(Global::sleepBeforeRestart_ms);
		if (!Global::OpenUdpSockets())
		{
			std::stringstream errorText;
			errorText << "Unable to reopen UDP sockets " << __FILE__;

			Debug::Write(errorText);
		}
	}
}


THREAD_RUN_RETURN_TYPE TimeThreadFunction(void*)
{
	for(;;)
	{
		try
		{
			Snooze(Global::timeThreadTickPeriod_ms);

			uint64 now_ms = GetMsSinceStart();

			Global::frameReceptionList.Tick(now_ms);
			
			Global::joinController.Tick();
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
			errorText << "Time thread Unknown exception caught " << __FILE__;

			Debug::Write(errorText);
		}
	}
}

THREAD_RUN_RETURN_TYPE GatewayTransmitThreadFunction(void* x)
{
	for(;;)
	{
		try
		{
			uint64 now_ms = GetMsSinceStart();

			Global::transmitQueue.Tick(now_ms);

			uint64 nextTxTime_ms = Global::transmitQueue.TimeOfNextTransmit_ms();
			now_ms = GetMsSinceStart();		// read again in case of delay due to mutex lock

			//Set snoozeTime to the shorter of the tick period and the delay to the next transmit time
			sint64 snoozeDelay_ms;
			if (nextTxTime_ms != invalidTime)
			{
				snoozeDelay_ms = static_cast<sint64>(nextTxTime_ms - now_ms);
				if (snoozeDelay_ms > static_cast<sint64>(Global::timeThreadTickPeriod_ms))
					snoozeDelay_ms = Global::timeThreadTickPeriod_ms;
			}
			else
				snoozeDelay_ms = Global::timeThreadTickPeriod_ms;

			if (snoozeDelay_ms > 0)
				Snooze(static_cast<uint>(snoozeDelay_ms));
		}
		catch (Exception const& e)
		{
			if (e.Level() >= Debug::GetPrintLevel())
			{
				std::stringstream errorText;
				
				errorText << "Gateway transmit thread exception : " << e.Filename() << ":" << e.Line() << ": " << e.Explanation();

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
			errorText << "Gateway transmit thread Unknown exception caught " << __FILE__;

			Debug::Write(errorText);
		}
	}
}

