/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "Service.hpp"
#include "Utilities.hpp"
#include <sstream>

const Service::Mask Service::userDataServer					= 1 << 0;
const Service::Mask Service::moteTxServer					= 1 << 1;
const Service::Mask Service::gatewayRxServer				= 1 << 2;
const Service::Mask Service::joinServer						= 1 << 3;
const Service::Mask Service::joinMonitor					= 1 << 4;
const Service::Mask Service::downstreamServer				= 1 << 5;

const Service::Mask Service::macCommandServer				= 1 << 6;
const Service::Mask Service::gatewayStatusServer			= 1 << 7;

const Service::Mask Service::maxMask						= Service::gatewayStatusServer;

const Service::Mask Service::nullMask						= Service::Mask(0);
const Service::Mask Service::fullMask						= ~Service::Mask(0);
const Service::Mask Service::errorMask						= Service::fullMask;

const Service::Mask Service::applicationServerMask	= Service::userDataServer | Service::moteTxServer | Service::gatewayRxServer | Service::joinServer;
const Service::Mask Service::customerServerMask		= Service::userDataServer | Service::moteTxServer | Service::gatewayRxServer | Service::joinMonitor;
const Service::Mask Service::networkControllerMask	= Service::moteTxServer | Service::gatewayRxServer | Service::macCommandServer;
const Service::Mask Service::downstreamServerMask	= Service::downstreamServer;

std::string Service::Text(Mask mask)
{
	switch (mask)
	{
	case Service::userDataServer:					return "user";
	case Service::moteTxServer:						return "motetx";
	case Service::gatewayRxServer:					return "gwrx";
	case Service::joinServer:						return "joinserver";
	case Service::joinMonitor:						return "joinmonitor";
	case Service::downstreamServer:					return "downstream";
	case Service::macCommandServer:					return "maccmd";
	case Service::gatewayStatusServer:				return "gwst";

	default:										return "ServiceTextError";
	}
}


std::string Service::TextString(Mask mask)
{
	std::string result;
	bool firstElement= true;

	for (Service::Mask test = 1; test <= Service::maxMask; test <<= 1)
	{
		if (mask & test)
		{
			if (firstElement)	// first loop
				firstElement = false;
			else
				result += ' ';

			result += Text(test);
		}
	}
	return result;
}


Service::Mask Service::ReadMask(std::string input)
{
	Service::Mask result = 0;
	std::vector<std::string> words;

	ConvertStringToWordVector(input, words);

	for (uint word = 0; word < words.size(); word++)
	{
		for (Mask test = 1; test <= Service::maxMask; test <<= 1)
		{
			if (words[word] == Text(test))
				result |= test;
		}
	}
	return result;
}
