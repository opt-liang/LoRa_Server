/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "SqlDb.hpp"

#ifdef _MSC_VER
	#include <windows.h>
	#include <winsock.h>
	#include "mysql.h"
#else
	#include <mysql/mysql.h>
#endif

#include "Utilities.hpp"

namespace SqlDb
{
	//lint -sem(SqlDb::MySqlClientData::FreeResult,cleanup)
	struct MySqlServerData
	{
	public:
		MYSQL		db;
	};

	struct MySqlClientData
	{
		MYSQL_ROW	row;		// do not delete/free as this is a pointer into result
		MYSQL_RES*	result;

		MySqlClientData() : row(0), result(0) {}
		~MySqlClientData() 
		{
			FreeResult();
		}

		void FreeResult()
		{
			if (result)
			{
				mysql_free_result(result);
				result = 0;
			}
			row = 0;
		}
	};
}


SqlDb::Server::Server()
	: mysql(new MySqlServerData),
	port(0), connected(false)
{
}


SqlDb::Server::~Server(void)
{
	Disconnect();
	delete mysql;
}


bool SqlDb::Server::Connect(std::string const& myHostName, std::string const& myDatabaseName, std::string const& myUserName, std::string const& myPassword, uint16 myPort)
{
	hostName = myHostName;
	userName = myUserName;
	passwd = myPassword; 
	databaseName = myDatabaseName;
	port = myPort;

	return Connect();
}

bool SqlDb::Server::Connect()
{
	mysql_init(&mysql->db);

	bool result = mysql_real_connect(&mysql->db, hostName.c_str(), userName.c_str(), passwd.c_str(), 
		databaseName.c_str(), port, 0, 0) == &mysql->db;

	if (result)
		result = (mysql_autocommit(&mysql->db, 1) == 0) ? true : false;

	connected = result;

	return result;
}


void SqlDb::Server::Disconnect()
{
	if (connected)
		mysql_close(&mysql->db);

	connected = false;
}


bool SqlDb::Server::IsConnected() const
{
	return connected && (mysql_ping(&mysql->db) == 0);
}


char const* SqlDb::Server::ErrorText() const
{
	return mysql_error(&mysql->db);
}


uint SqlDb::Server::ErrorNumber() const
{
	return mysql_errno(&mysql->db);
}


bool SqlDb::Server::QueryPrivate(Client& client, char const input[], bool storeResult, bool firstCall)
{
	if (Debug::Print(Debug::verbose))
	{
		std::stringstream text;

		text << "SQL query = " << input;
		Debug::Write(text);
	}

	int sqlResult = mysql_query(&mysql->db, input);

	if (sqlResult != 0)
	{
		std::stringstream text;

		text << "SQL library reported query error #" << mysql_error(&mysql->db);
		Debug::Write(text);

		Disconnect();			//Disconnect and reconnect - in order to fix the problem
		bool reconnected = Connect();

		if (reconnected)
		{
			sqlResult = mysql_query(&mysql->db, input);

			if (sqlResult != 0)
			{
				text.str("");	//empty string
				text << "SQL library reported repeat query error #" << mysql_error(&mysql->db);
				Debug::Write(text);
				return false;
			}
		}
		else
		{
			Debug::Write("Unable to reconnect to database");
			return false;
		}
	}

	bool success = true;

	if (storeResult)
		success = client.GetResult();

	return success;
}


bool SqlDb::Server::Query(Client& client, char const input[], bool storeResult)
{
	MutexHolder holder(mutex);
	return QueryPrivate(client, input, storeResult);
}

bool SqlDb::Server::Query(char const input[])
{
	SqlDb::Client client(*this);

	bool result = true;
	try
	{
		client.Query(input, false);
	}
	catch (SqlDb::Exception const&)
	{
		result = false;
	}

	return result;
}


uint64 SqlDb::Server::QueryReportingAutoIncrementValue(Client& client, char const input[])
{
	uint64 result = invalidSqlIndex;

	MutexHolder holder(mutex);	//mutex held in this function to ensure mysql_insert_id refers to this query
	bool success = QueryPrivate(client, input, false);

	if (success)
		result = mysql_insert_id(&mysql->db);

	return result;
}


uint64 SqlDb::Server::QueryReportingAutoIncrementValue(char const input[])
{
	SqlDb::Client client(*this);

	uint64 result = SqlDb::invalidSqlIndex;
	try
	{
		result = client.QueryReportingAutoIncrementValue(input);
	}
	catch (SqlDb::Exception const&)
	{
	}
	return result;
}


bool SqlDb::Server::ColumnExists(std::string const& table, std::string const& column)
{
	std::stringstream query;

	query << "SELECT * FROM INFORMATION_SCHEMA.COLUMNS WHERE `TABLE_NAME` = \"" << table << "\" AND `COLUMN_NAME` = \"" << column << "\"";

	ClientSingleQuery client(*this, query, true);

	return (client.RowsInResult() == 1) ? true : false;
}

bool SqlDb::Server::AddColumn(std::string const& table, std::string const& column, std::string const& description, std::string const& positionInTable)
{
	std::stringstream query;

	query << "ALTER TABLE " << table << " ADD " << column << " " << description << " " << positionInTable;

	return Query(query);
}


bool SqlDb::Server::DropColumn(std::string const& table, std::string const& column)
{
	std::stringstream query;

	query << "ALTER TABLE " << table << " DROP " << column;

	return Query(query);
}


bool SqlDb::Server::AddColumnIfNotPresent(std::string const& table, std::string const& column, std::string const& description, std::string const& positionInTable)
{
	if (ColumnExists(table, column))
		return true;

	return AddColumn(table, column, description, positionInTable);
}


bool SqlDb::Server::DropColumnIfPresent(std::string const& table, std::string const& column)
{
	if (!ColumnExists(table, column))
		return true;

	return DropColumn(table, column);
}


void SqlDb::Server::EmptyTable(const char table[])
{
	std::stringstream query;
	
	query << "DELETE FROM " << table;
	Query(query);
}


SqlDb::Client::Client(SqlDb::Server& myServer)
	:server(myServer), data(new MySqlClientData)
{
}


SqlDb::Client::~Client()
{
	FreeResult();

	delete data;
}


bool SqlDb::Client::GetResult()
{
	FreeResult();

	data->result = mysql_store_result(&server.mysql->db);

	return (data->result != 0) ? true : false;
}


bool SqlDb::Client::GetNextRow()
{
	if (!data->result)
		return false;

	data->row = mysql_fetch_row(data->result);

	bool result = data->row ? true : false;
	if (!data->row)
		FreeResult();

	return result;
}


uint64 SqlDb::Client::RowsInResult() const
{
	if (!data->result)
		return 0;

	return mysql_num_rows(data->result);
}


uint16 SqlDb::Client::FieldsInResult() const
{
	if (!data->result)
		return 0;

	return mysql_num_fields(data->result);
}


void SqlDb::Client::FreeResult()
{
	data->FreeResult();
}


char const* SqlDb::Client::GetFieldValue(uint16 field) const
{
	static char const nulltext[] = "";

	if (data->row == 0)
		return nulltext;

	char const* result = data->row[field];

	if (!result)
		return nulltext;

	return result;
}

