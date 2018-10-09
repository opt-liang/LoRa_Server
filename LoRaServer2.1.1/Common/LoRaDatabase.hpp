/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef LORA_DATABASE_HPP
#define LORA_DATABASE_HPP

#include "Eui.hpp"
#include "SqlDb.hpp"
#include <string>
#include <stdlib.h> //needed by linux for strtol

namespace LoRa
{
	class Database //throws exception
	{
	protected:
		SqlDb::Server	db;

	public:
		Database() {}
		virtual ~Database() {}

		bool Connect(std::string const& myHostName, std::string const& myDatabaseName, std::string const& myUserName, std::string const& myPassword, uint16 myPort = SqlDb::Server::defaultPort)
		{return db.Connect(myHostName, myDatabaseName, myUserName, myPassword, myPort);}

		bool IsConnected() const {return db.IsConnected();}
		bool Reconnect() {return db.Reconnect();}
		char const* ErrorText() const {return db.ErrorText();}
		uint ErrorNumber() const {return db.ErrorNumber();}
		void UpdateBuildTime();

		bool SetConfiguredValue(const char name[], const char value[]);
		bool SetConfiguredValue(const char name[], std::string const& value) {return SetConfiguredValue(name, value.c_str());}
		bool SetConfiguredValue(const char name[], std::stringstream const& value) {return SetConfiguredValue(name, value.str());}
		bool GetConfiguredValue(const char name[], std::string& value);	//returns false on error
		std::string GetConfiguredValue(const char name[]);	//returns empty string on error
		void EmptyTable(char const table[]) {db.EmptyTable(table);}

		operator SqlDb::Server&() {return db;}

		bool UpdateStructure();		//called by connect in order to align DB structure to current format

	private:
		bool SetServersAppEuiToUnsigned() {return db.Query("ALTER TABLE `servers` CHANGE `appEui` `appEui` BIGINT(20) UNSIGNED NOT NULL");}
		virtual bool UpdateDBSpecificStructure() {return true;}		//called by UpdateStructure() in order to align DB structure to current format
	};
}

#endif

