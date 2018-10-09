/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef LORA_APPLICATION_SERVER_DATABASE_HPP
#define LORA_APPLICATION_SERVER_DATABASE_HPP

#include "Eui.hpp"
#include "SqlDb.hpp"
#include "LoRaDatabase.hpp"
#include "LoRaApplicationDatabase.hpp"
#include "LoRa.hpp"
#include "SqlDb.hpp"
#include <string>

namespace LoRa
{
	class DatabaseAS : public LoRa::ApplicationDatabase//throws exception
	{
		// LoRa::ApplicationDatabase contains LoRa::Database as a virtual base class
	public:
		struct JoinMoteRecord
		{
			EuiType				moteEui;
			EuiType				appEui;
			LoRa::CypherKey		applicationKey;
		};

		class JoinMoteClient : public SqlDb::ClientSingleQuery
		{
		public:
			JoinMoteClient(SqlDb::Server& myServer)
				: SqlDb::ClientSingleQuery(myServer, "SELECT eui, appEui, appKey FROM joinmotes ORDER BY eui ASC", true)
			{}

			JoinMoteRecord Read();	// returns an all ones eui when no more to be read
		};

		struct ActiveMoteRecord
		{
			EuiType				moteEui;
			EuiType				appEui;
			LoRa::CypherKey		sessionKey;
			uint32				networkAddress;
			LoRa::CypherKey		applicationKey;	//will be invalid if mote is not a join (OTA) mote
		};

		class ActiveMoteClient : public SqlDb::ClientSingleQuery
		{
		public:
			ActiveMoteClient(SqlDb::Server& myServer)
				: SqlDb::ClientSingleQuery(myServer, "SELECT activemotes.eui, activemotes.appEui, sessionKey, networkAddress, appKey FROM activemotes LEFT JOIN joinmotes ON activemotes.eui = joinmotes.eui ORDER BY activemotes.eui ASC", true)
			{}

			ActiveMoteRecord Read();	// returns an all ones eui when no more to be read
		};


		DatabaseAS()
		{}

		bool WriteJoinMote(EuiType moteEui, EuiType applicationEui, LoRa::CypherKey const& applicationKey);
		bool MoteNonceKnown(EuiType moteEui, uint16 deviceNonce);
		bool DeleteJoinMote(EuiType moteEui);
		bool JoinMoteExists(EuiType moteEui);
		bool FindJoinMoteApplicationKey(EuiType moteEui, EuiType applicationEui, LoRa::CypherKey& applicationKey);
		void DeleteAllJoinMotes() {db.EmptyTable("joinmotes");}
		void DeleteAllNoncesBelongingToMote(EuiType eui);

		bool CreateActiveMote(EuiType moteEui, EuiType appEui, LoRa::CypherKey const& sessionKey, uint32 networkAddress);
		bool DeleteActiveMote(EuiType moteEui);
		void DeleteAllActiveMotes() {db.EmptyTable("activemotes");}


		void DeleteAllNonces() {db.EmptyTable("nonces");}
	};
}

#endif

