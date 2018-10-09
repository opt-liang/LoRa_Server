/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef LORA_APPLICATION_DATABASE_HPP
#define LORA_APPLICATION_DATABASE_HPP

#include "Eui.hpp"
#include "SqlDb.hpp"
#include "LoRaDatabase.hpp"
#include "LoRa.hpp"
#include "Application.hpp"
#include <string>

namespace LoRa
{
	class ApplicationDatabase : public virtual LoRa::Database //throws exception
	{
	private:
		static char const sqlDotSubstitutionCharacter = '@';	//needed because SQL treats '.' characters as word delimiters when searching for text

	public:
		struct ApplicationRecord
		{
			EuiType			eui;	// in BinarySearchVector::ElementTemplate<Client, EuiType> this is Id()
			std::string 	name;
			std::string 	owner;
			void SetInvalid() {eui = invalidEui; name == ""; owner = "";};
			bool IsValid() const {return eui != invalidEui;}
		};

		struct ServerRecord
		{
			sockaddr_in				address;
			Service::Mask			mask;
			bool					active;

			void SetInvalid() {::SetInvalid(address); mask = 0;}
			bool IsValid() const {return ::IsValidPort(address);}
		};

		class ApplicationClient : public SqlDb::ClientSingleQuery
		{
		public:
			ApplicationClient(SqlDb::Server& myServer)
				: SqlDb::ClientSingleQuery(myServer, "SELECT eui, name, owner FROM applications ORDER BY eui ASC", true)
			{}

			ApplicationRecord Read();	// returns an all ones address when no more to be read
		};

		class ServerClient : public SqlDb::Client
		{
		public:
			ServerClient(SqlDb::Server& myServer, EuiType eui = invalidEui)
				: SqlDb::Client(myServer)
			{
				std::stringstream query;

				query << "SELECT addressText, active, serviceText FROM servers WHERE ";

				if (eui != invalidEui)
					query << "HEX(appEui) = \'" << std::hex << eui << "\' AND nullApplication = 0";
				else
					query << "nullApplication = 1";

				Query(query, true);
			}

			virtual ~ServerClient() {}

			ServerRecord Read();	// returns an invalid address when no more to be read
		};

		ApplicationDatabase()
		{}

		bool CreateApplication(::EuiType eui, std::string const& name, std::string const& owner);
		bool DeleteApplication(::EuiType eui);

		bool CreateApplicationServer(::EuiType eui, sockaddr_in const& address, bool active, Service::Mask actionMask);
		bool UpdateApplicationServer(::EuiType eui, sockaddr_in const& address, Service::Mask actionMask);

		bool DeleteApplicationServers(::EuiType eui);
		bool DeleteApplicationServer(::EuiType eui, sockaddr_in const& serverAddress);
		void DeleteAllApplications() {db.EmptyTable("servers"); db.EmptyTable("applications");}

	private:
		static std::string ConvertPortAddressToSqlSafeString(sockaddr_in const& address);	//required for fields that are searched on because '.' characters are treated specially in SQL
		static sockaddr_in ReadPortAddressFromSqlSafeString(char const text[]);
		inline static sockaddr_in ReadPortAddressFromSqlSafeString(std::string const& text) {return ReadPortAddressFromSqlSafeString(text.c_str());}
	};
}
#endif

