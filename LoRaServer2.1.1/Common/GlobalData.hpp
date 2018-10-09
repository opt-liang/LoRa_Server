/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef GLOBAL_DATA_HPP
#define GLOBAL_DATA_HPP

#include "General.h"
#include "Application.hpp"
#include "UdpSocket.hpp"
#include "TcpSocket.hpp"
#include "IpSocketSet.hpp"
#include "TcpTextSocket.hpp"
#include "ConfiguredValue.hpp"
#include "CommandLineInterface.hpp"
#include "TcpTextSocketConnectionManager.hpp"
#include "TcpConnectionAddressController.hpp"
#include "LoRaApplicationDatabase.hpp"

namespace Global
{
	// Data that is common to all servers
	extern const char							programDescription[];
	extern std::string							databaseHost;
	extern std::string							databaseName;

	const uint32 								timeThreadTickPeriod_ms = 50;
	const uint32 								connectThreadTickPeriod_ms = 1000;
	const uint32								sleepBeforeRestart_ms = 1000;

	const uint32 								commandLineInterfaceClientKeepAlivePeriod_ms = 5 * 60 * 1000;
	const uint16								maxReceivedIpMessage = 2 * 1024;
	const uint16								receiveBufferLength = maxReceivedIpMessage  +1;

	extern uint16								localIpReceivePort;
	extern UDP::Socket							udpSocket;
	extern TCP::ServerSocket					tcpServerSocket;

	extern TCP::TextConnectionManager			tcpConnectionManager;
	extern TCP::ConnectionAddressController		tcpConnectionAddressController;
	extern Application::List					applicationList;

	extern CommandLineInterface::Server			commandLineInterface;

	extern LoRa::ApplicationDatabase&			genericDatabase;	//only a reference because the database is specifised by each server

	extern ConfiguredBool						allowRemoteConfiguration;
	extern ConfiguredBool						autoCreateMotes;

	void InitialiseData();	//Called after DB is active
	void InitialiseDataServerSpecific();	// Called only by InitialiseData() - defined in each server
	void ServerSpecificTick();	//called by main time thread
	void DeleteAllData();
	void DeleteAllDataServerSpecific();	// Called only by InitialiseData() - defined in each server

	bool OpenUdpSockets();	// opens  all UDP sockets - defined for each server
	void CloseUdpSockets();	// closes all UDP sockets - defined for each server
}
#endif

