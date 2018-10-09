/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef LORA_NETWORK_SERVER_DATABASE_HPP
#define LORA_NETWORK_SERVER_DATABASE_HPP

#include "Eui.hpp"
#include "SqlDb.hpp"
#include "LoRaDatabase.hpp"
#include "LoRaApplicationDatabase.hpp"
#include "LoRa.hpp"
#include "GatewayStatus.hpp"
#include "Position.hpp"

namespace LoRa
{
	class DatabaseNS  : public LoRa::ApplicationDatabase //throws exception
	{
		// LoRa::ApplicationDatabase contains LoRa::Database as a virtual base class
	public:
		struct MoteRecord
		{
			EuiType					moteEui;
			EuiType					appEui;
			uint32					networkAddress;
			LoRa::CypherKey			networkSessionKey;
			uint32					upMessageSequenceNumber;	//The current up (from mote to server) sequence number
			uint32					downMessageSequenceNumber;	//The current down (from server to mote) sequence number
		};

		struct GatewayRecord
		{
			EuiType					eui;
			LoRa::Region			region;	//region == numberOfRegions if not read from DB
			bool					allowGpsToSetPosition;
			Position				position;
			GatewayFrameCountType	frameCount;
		};

		class MoteClient : public SqlDb::ClientSingleQuery
		{
		public:
			MoteClient(SqlDb::Server& myServer)
				: SqlDb::ClientSingleQuery(myServer, 
				"SELECT eui, appeui, networkAddress, networksessionkey, downMsgSeqNo, upmsgseqno FROM motes", true)
			{}

			MoteRecord Read();	// returns eui == invalidEui when no more to be read
		};

		class GatewayClient : public SqlDb::ClientSingleQuery
		{
		public:
			GatewayClient(SqlDb::Server& myServer)
				: SqlDb::ClientSingleQuery(myServer,
				"SELECT eui, allowGpsToSetPosition, region, latitude, longitude, altitude, uppacketsreceived, gooduppacketsreceived, uppacketsforwarded, uppacketsacknowedgedratio, downpacketsreceived, \
				packetstransmitted FROM gateways ORDER BY eui ASC", true)
			{}

			GatewayRecord Read();	// returns an all ones address when no more to be read
		};

		DatabaseNS()
		{}

		bool CreateMote(EuiType moteEui, EuiType appEui, uint32 networkAddress, LoRa::CypherKey const& networkSessionKey, uint32 upMessageSequenceNumber, uint32 downMessageSequenceNumber);
		bool DeleteMote(EuiType moteEui);
		void DeleteAllMotes() {db.EmptyTable("motes");}
		bool UpdateMoteSequenceNumbers(EuiType moteEui, uint32 downstreamSequenceNumber, uint32 upstreamSequenceNumber);

		bool CreateGateway(EuiType eui, LoRa::Region region, bool allowGpsToSetPosition);
		bool UpdateGateway(EuiType eui, LoRa::Region region);
		bool UpdateGateway(EuiType eui, GatewayFrameCountType const& count);
		bool UpdateGateway(EuiType eui, Position const& position);
		bool UpdateGateway(EuiType eui, TimeRecord const& time);
		bool UpdateGatewayAllowGpsToSetPosition(EuiType eui, bool allowGpsToSetPosition);
		bool DeleteGateway(EuiType eui);
		void DeleteAllGateways()  {db.EmptyTable("gateways");}

	private:
		bool AddGatewayRegionColumn() {return db.AddColumnIfNotPresent("gateways", "region", "TINYINT UNSIGNED NULL DEFAULT NULL COMMENT 'Enumerated type 0=americas902, 1=china779, 2=europe433, 3=europe863'", "AFTER eui");}
		bool AddGatewayAllowGPSToSetPositionColumn() {return db.AddColumnIfNotPresent("gateways", "allowGpsToSetPosition", "BOOLEAN NOT NULL DEFAULT TRUE", "AFTER eui");}

		//Redefined virtual function
		bool UpdateDBSpecificStructure();
	};
}
#endif

