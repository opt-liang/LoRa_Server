/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef DEBUG_MONITOR
#define DEBUG_MONITOR

#include "General.h"
#include "Mutex.hpp"

#include <fstream>
#include <ostream>
#include <string>
#include <sstream>

namespace Debug
{
	enum Level {major, minor, monitor, verbose};
	void SetConsoleDisplay(bool on);
	bool UseConsole();

	class Log
	{
	private:
		Level printLevel;
		std::string filename;
		std::ofstream file;
		Mutex	mutex;

	public:
		Log(Level myPrintLevel)
			: printLevel(myPrintLevel)
		{}

		bool SetFile(char const name[]);
		bool SetFile(std::string const& name) {return SetFile(name.c_str());}
		void SetPrintLevel(Level newLevel) {printLevel = newLevel;}
		bool SetPrintLevel(std::string const& levelText);
		Level GetPrintLevel() const {return printLevel;}
		bool FlushFile();

		bool Print(Level l) const {return l <= printLevel;}

		void Write(std::string const& text);
		void Write(std::stringstream const& text) {Write(text.str());}

		void FileWrite(std::string const& text);	//never writes to console
		void FileWrite(std::stringstream const& text) {FileWrite(text.str());}

	private:
		static std::string Time();
		bool FlushFilePrivate();
	};

	extern Log log;
	inline bool SetFile(char const name[]) {return log.SetFile(name);}
	inline bool SetFile(std::string const& name) {return log.SetFile(name);}
	inline bool Print(Level l) {return log.Print(l);}
	inline void SetPrintLevel(Level newLevel) {log.SetPrintLevel(newLevel);}
	inline Level GetPrintLevel() {return log.GetPrintLevel();}
	inline bool FlushFile() {return log.FlushFile();}
	inline bool SetPrintLevel(std::string const& levelText) {return log.SetPrintLevel(levelText);}

	inline void Write(std::string const& text) {log.Write(text);}
	inline void Write(std::stringstream const& text) {Write(text.str());}
	inline void FileWrite(std::string const& text) {log.FileWrite(text);}
	inline void FileWrite(std::stringstream const& text) {FileWrite(text.str());}
	void ConsoleWrite(std::string const& text);	//protected by mutex
	inline void ConsoleWrite(std::stringstream const& text) {ConsoleWrite(text.str());}
}

#endif

