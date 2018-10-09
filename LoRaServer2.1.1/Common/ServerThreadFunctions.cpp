/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "ServerThreadFunctions.hpp"
#include "GlobalData.hpp"
#include "TcpConnectionAddressController.hpp"
#include "TcpTextSocket.hpp"


THREAD_RUN_RETURN_TYPE JsonTcpWaitThreadFunction(void*)
{
	for (;;)
	{
		try
		{
			Global::tcpServerSocket.Open(Global::localIpReceivePort);

			for (;;)
			{
				sockaddr_in remotePort;
				//lint -e{429}  (Warning -- Custodial pointer 'newSocket' (line 231) has not been freed or returned)
				SOCKET socketDescriptor = Global::tcpServerSocket.WaitForConnection(remotePort);

				if (socketDescriptor == INVALID_SOCKET)
				{
					if ((IP::lastError.Number() == IP::Error::notASocket) || (IP::lastError.Number() == IP::Error::badFile))
					{
						Snooze(Global::connectThreadTickPeriod_ms / 2);	//one of the connections has closed - sleep for roughly the period of the checker thread and retry
						continue;
					}
					else
						throw Exception(Debug::major, __LINE__,__FILE__, IP::lastError.Text(), IP::lastError.Number());
				}

				uint32 connectionId = Global::tcpConnectionManager.ReceivedConnection(socketDescriptor, remotePort);

				Global::tcpConnectionAddressController.NewConnectionReceived(connectionId);

				if (Debug::Print(Debug::monitor))
				{
					std::stringstream text;

					text << "Now connected to IP host " << AddressText(remotePort) << " via Connection " << connectionId;

					Debug::Write(text);
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
		catch (...)
		{
			std::stringstream errorText;
			errorText << "JsonTcpWaitThreadFunction Unknown exception caught " << __FILE__;

			Debug::Write(errorText);
		}
		Snooze(Global::sleepBeforeRestart_ms);
	}
}


THREAD_RUN_RETURN_TYPE JsonTcpReceiveThreadFunction(void*)
{
	for (;;)
	{
		try
		{
			for (;;)
			{
				TCP::TextSocket* tcpSocket = static_cast<TCP::TextSocket*>(Global::tcpConnectionManager.Wait());

				if (!tcpSocket)
					throw Exception(Debug::major, __LINE__,__FILE__, IP::lastError.Text(), IP::lastError.Number());

				TCP::Connection::IdType connectionId = Global::tcpConnectionManager.GetConnectionId(*tcpSocket);
				if (connectionId == TCP::Connection::invalidId)
					continue;	//connection has been removed from manager

				MessageAddress source(connectionId);
				const int receiveBufferLength = Global::maxReceivedIpMessage+1;
				uint8 receivedData[receiveBufferLength];

				do
				{
					int receivedLength = tcpSocket->Receive(receivedData, receiveBufferLength);

					if (receivedLength >= receiveBufferLength)
						Debug::Write("Overlong TCP message received");

					else if (receivedLength > 0)
						JSON::Receive::Top((char const*) receivedData, source, true);
				} while (tcpSocket->ProtocolMessageWaiting());
			}
		}
		catch (Exception const& e)
		{
			std::stringstream errorText;

			errorText << "Exception : " << e.Filename() << ":" << e.Line() << ": " << e.Explanation();

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
		catch (...)
		{
			std::stringstream errorText;
			errorText << "JsonTcpReceiveThreadFunction Unknown exception caught " << __FILE__;

			Debug::Write(errorText);
		}
		Snooze(Global::sleepBeforeRestart_ms);
	}
}


THREAD_RUN_RETURN_TYPE TcpConnectThreadFunction(void* x)
{
	for(;;)
	{
		try
		{
			Snooze(Global::connectThreadTickPeriod_ms);

			Global::tcpConnectionManager.RefreshConnections();
		}
		catch (Exception const& e)
		{
			if (e.Level() >= Debug::GetPrintLevel())
			{
				std::stringstream errorText;
				
				errorText << "TCP connect thread exception : " << e.Filename() << ":" << e.Line() << ": " << e.Explanation();

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
			errorText << "TCP connect std::exception " << e.what();

			Debug::Write(errorText);
		}
		catch (...)
		{
			std::stringstream errorText;
			errorText << "TCP connect Unknown exception caught " << __FILE__;

			Debug::Write(errorText);
		}
	}
}


THREAD_RUN_RETURN_TYPE UdpReceiveThreadFunction(void*)
{
	for (;;)
	{
		try
		{
			for (;;)
			{
				const int receiveBufferLength = Global::maxReceivedIpMessage+1;
				uint8 receivedData[receiveBufferLength];
				sockaddr_in source;

				int receivedLength = Global::udpSocket.Receive(source,receivedData, receiveBufferLength);

				if (receivedLength <= 0)
				{
					if (receivedLength < 0)
						throw Exception(Debug::major, __LINE__, __FILE__, IP::lastError.Text(), IP::lastError.Number());
					else
						continue;
				}
				else if (receivedLength >= receiveBufferLength)
				{
					Debug::Write("Overlong UDP message received");
					continue;
				}

				receivedData[receivedLength] = '\0'; // add a NULL to the end 

				JSON::Receive::Top((char const*) receivedData, source, false);
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

