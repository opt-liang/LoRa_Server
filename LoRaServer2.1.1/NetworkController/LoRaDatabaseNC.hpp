/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef NETWORK_CONTROLLER_DATABASE_HPP
#define NETWORK_CONTROLLER_DATABASE_HPP

#include "Eui.hpp"
#include "LoRaDatabase.hpp"
#include "LoRaApplicationDatabase.hpp"
#include "LoRa.hpp"
#include "SqlDb.hpp"

namespace LoRa
{
	class DatabaseNC : public LoRa::ApplicationDatabase//throws exception
	{
		// LoRa::ApplicationDatabase contains LoRa::Database as a virtual base class
	public:
		struct MoteRecord
		{
			EuiType				moteEui;
			EuiType				appEui;
		};

		class MoteClient : public SqlDb::ClientSingleQuery
		{
		public:
			MoteClient(SqlDb::Server& myServer)
				: SqlDb::ClientSingleQuery(myServer, "SELECT eui, appEui FROM motes ORDER BY eui ASC", true)
			{}

			MoteRecord Read();	// returns an all ones eui when no more to be read
		};


		DatabaseNC()
		{}

		bool CreateMote(EuiType moteEui, EuiType applicationEui);
		bool DeleteMote(EuiType moteEui);
		void DeleteAllMotes() {db.EmptyTable("motes");}
	};
}

#endif

