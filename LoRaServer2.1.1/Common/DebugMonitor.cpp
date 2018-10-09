/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "DebugMonitor.hpp"

#include "Utilities.hpp"

#include <iostream>
#include <iomanip>
#include <time.h>
#include "TimeFunctions.hpp"

Debug::Log Debug::log(Debug::major);

namespace
{
	char const* const month[] =
	{
		"January", "February", "March", "April", "May", "June", 
		"July", "August", "September", "October", "November", "December"
	};
	bool useConsole = false;
	Mutex consoleMutex;
}


std::string Debug::Log::Time()
{
	time_t rawtime;

	time (&rawtime);

	struct tm time = gmtimeSafe(rawtime);

	std::stringstream result;

	result << 
		std::setfill('0') << std::setw(2LL) << time.tm_hour << ':' <<
		std::setfill('0') << std::setw(2LL) << time.tm_min << ':' <<
		std::setfill('0') << std::setw(2LL) << time.tm_sec << "  " << 

		std::setw(4) << (GetMsSinceStart() % 10000) << "  " << std::setw(0) <<

		time.tm_mday << ' ' <<
		month[time.tm_mon] << ' ' << 
		time.tm_year + 1900;

	return result.str();
}


void Debug::Log::FileWrite(std::string const& text)
{
	MutexHolder holder(mutex);
	if (file.is_open())
	{
		file << Time() << ": " << text << std::endl;
		file.flush();
	}
}

void Debug::Log::Write(std::string const& text)
{
	FileWrite(text);

	if (useConsole)
	{
		std::stringstream local;
		local << text << std::endl;

		ConsoleWrite(local);
	}
}

bool Debug::Log::SetFile(char const name[])
{
	MutexHolder holder(mutex);
	if (file.is_open())
		file.close();

	filename = name;

	return FlushFilePrivate();
}

bool Debug::Log::FlushFile()
{
	MutexHolder holder(mutex);
	return FlushFilePrivate();
}

bool Debug::Log::FlushFilePrivate()
{
	if (file.is_open())
		file.close();

	if (!filename.empty())
		file.open(filename.c_str()); 

	return !file.fail();
}

bool Debug::Log::SetPrintLevel(std::string const& levelText)
{
	if (levelText == "major")
		SetPrintLevel(Debug::major);

	else if (levelText == "minor")
		SetPrintLevel(Debug::minor);

	else if (levelText == "monitor")
		SetPrintLevel(Debug::monitor);

	else if (levelText == "verbose")
		SetPrintLevel(Debug::verbose);
	else
		return false;

	return true;
}


void Debug::SetConsoleDisplay(bool on)
{
	useConsole = on;
}

bool Debug::UseConsole()
{
	return useConsole;
}


void Debug::ConsoleWrite(std::string const& text)
{
	MutexHolder holder(consoleMutex);
	std::cerr << text;
}

