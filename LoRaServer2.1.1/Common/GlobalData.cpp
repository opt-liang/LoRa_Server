/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "GlobalData.hpp"

namespace Global
{
	uint16								localIpReceivePort = 0;
	UDP::Socket							udpSocket;
	TCP::ServerSocket					tcpServerSocket(maxReceivedIpMessage);

	TCP::TextConnectionManager			tcpConnectionManager(Global::timeThreadTickPeriod_ms, Global::maxReceivedIpMessage, true);	//use round robin servicing
	TCP::ConnectionAddressController	tcpConnectionAddressController(tcpConnectionManager);

	CommandLineInterface::Server		commandLineInterface(udpSocket, commandLineInterfaceClientKeepAlivePeriod_ms / timeThreadTickPeriod_ms);

	Application::List					applicationList(false);

	ConfiguredBool						autoCreateMotes("autoCreateMotes", false);
	ConfiguredBool						allowRemoteConfiguration("allowRemoteConfiguration", false);
}

