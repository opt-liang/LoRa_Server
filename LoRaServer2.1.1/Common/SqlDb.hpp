/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef MY_SQL_DB_HPP
#define MY_SQL_DB_HPP

#include "General.h"
#include "Mutex.hpp"
#include "ExceptionClass.hpp"

#include <string>
#include <sstream>

namespace SqlDb
{
	struct MySqlServerData;
	struct MySqlClientData;
	class Client;
	static const uint64 invalidSqlIndex = ~uint64(0); //all ones

	class Server
	{
		friend class Client;

	public:
		static const unsigned defaultPort = 3306;

	private:
		MySqlServerData*		mysql;		// pointer to avoid including mysql.h into this file
		Mutex					mutex;
		std::string				hostName;
		std::string				userName;
		std::string				passwd;
		std::string				databaseName;
		uint16					port;
		bool					connected; // true between Connect() and Disconnect() being called - but real DB may not actually be connected

	public:

		Server();

		Server(Server const& other);	//do not use or define - copy should not be done

		~Server(void);

		bool Connect(std::string const& myHostName, std::string const& myDatabaseName, std::string const& myUserName, std::string const& myPassword, uint16 myPort = defaultPort);	//return true on success
		void Disconnect();

		bool Reconnect()
		{
			Disconnect();
			return Connect();
		}

		bool IsConnected() const;
		char const* ErrorText() const;
		uint ErrorNumber() const;
		bool Query(Client& client, char const input[], bool storeResult);
		uint64 QueryReportingAutoIncrementValue(Client& client, char const input[]);	//returns the auto-incremented id of the added item

		//Only use these functions when result isn't needed
		bool Query(std::stringstream const& input) {return Query(input.str());}
		bool Query(std::string const& input) {return Query(input.c_str());}
		bool Query(char const input[]);

		//returns the auto-incremented id of the added item
		uint64 QueryReportingAutoIncrementValue(std::stringstream const& input) {return QueryReportingAutoIncrementValue(input.str());}
		uint64 QueryReportingAutoIncrementValue(std::string const& input) {return QueryReportingAutoIncrementValue(input.c_str());}
		uint64 QueryReportingAutoIncrementValue(char const input[]);

		bool ColumnExists(std::string const& table, std::string const& column);	//true if column exists in DB
		bool AddColumn(std::string const& table, std::string const& column, std::string const& description, std::string const& positionInTable = "");
		//Example AddColumn("tablename", "columnname", "This is a useful thing to store", "AFTER eui");
		bool DropColumn(std::string const& table, std::string const& column);	//returns true if the column was deleted

		bool AddColumnIfNotPresent(std::string const& table, std::string const& column, std::string const& description, std::string const& positionInTable = "");
		bool DropColumnIfPresent(std::string const& table, std::string const& column);	//returns true if the column does not exist OR was deleted

		void EmptyTable(const char table[]);

	private:
		bool QueryPrivate(Client& client, char const input[], bool storeResult, bool firstCall = true);
			//mutex must be locked when this function is called.  firstCall is true unless the method is called from Fail()

		bool Connect();
	};

	class Client	//client must not be shared between threads - unless protected
	{
		friend class Server;

	public:
		Client(Server& myServer);
		Client(Client const& other);	//do not use or define - copy should not be done
		virtual ~Client();

		bool Query(char const input[], bool storeResult) {return server.Query(*this, input, storeResult);}
		bool Query(std::string const& input, bool storeResult) {return Query(input.c_str(), storeResult);}
		bool Query(std::stringstream const& input, bool storeResult) {return Query(input.str(), storeResult);}
		uint64 QueryReportingAutoIncrementValue(char const input[]) {return server.QueryReportingAutoIncrementValue(*this, input);}	//returns the auto-incremented id of the added item
		uint64 QueryReportingAutoIncrementValue(std::string const& input) {return QueryReportingAutoIncrementValue(input.c_str());}	//returns the auto-incremented id of the added item
		uint64 QueryReportingAutoIncrementValue(std::stringstream const& input) {return QueryReportingAutoIncrementValue(input.str());}	//returns the auto-incremented id of the added item


		//Result methods - only valid if storeResult parameter of Query() is true
		bool GetNextRow();
		uint64 RowsInResult() const;
		uint16 FieldsInResult() const;
		char const* GetFieldValue(uint16 fieldNumber) const;
		char const* ErrorText() const {return server.ErrorText();}

	private:
		Server&				server;
		MySqlClientData*	data;

		bool GetResult();	//copies result to client's store - ONLY called from Query functions
		void FreeResult();
	};

	class ClientSingleQuery : public Client
	{
	public:
		ClientSingleQuery(Server& myServer, char const queryText[], bool storeResult) : SqlDb::Client(myServer), query(queryText)
		{Query(query, storeResult);}

		ClientSingleQuery(Server& myServer, std::string const& queryText, bool storeResult) : SqlDb::Client(myServer), query(queryText)
		{Query(query, storeResult);}

		ClientSingleQuery(Server& myServer, std::stringstream const& queryText, bool storeResult) : SqlDb::Client(myServer), query(queryText.str())
		{Query(query, storeResult);}

		virtual ~ClientSingleQuery() {}

	protected:
		std::string query;
	};

	class Exception : public ::Exception
	{
	public:
		Exception(Server const& server, Debug::Level myLevel, unsigned myLine, char const myFilename[])
			: ::Exception(myLevel, myLine, myFilename, server.ErrorText())
		{
		}
	};
}

#endif
