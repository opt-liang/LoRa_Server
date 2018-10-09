/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef SERVER_THREADS_HPP
#define SERVER_THREADS_HPP

#include "General.h"
#include "ExceptionClass.hpp"
#include "JsonReceive.hpp"
#include "Thread.hpp"
#include "Utilities.hpp"
#include "DebugMonitor.hpp"
#include "InputError.hpp"
#include "LoRa.hpp"
#include "LoRaIpPorts.hpp"

#include <stdio.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <signal.h>


THREAD_RUN_RETURN_TYPE JsonTcpWaitThreadFunction(void* x = 0);
THREAD_RUN_RETURN_TYPE JsonTcpReceiveThreadFunction(void* x = 0);
THREAD_RUN_RETURN_TYPE TcpConnectThreadFunction(void* x = 0);
THREAD_RUN_RETURN_TYPE UdpReceiveThreadFunction(void* x = 0);	//receives upd messages (mainly from the command console)


#endif

