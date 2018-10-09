/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef CUSTOMER_SERVER_DATABASE_HPP
#define CUSTOMER_SERVER_DATABASE_HPP

#include "Eui.hpp"
#include "LoRaDatabase.hpp"
#include "LoRaApplicationDatabase.hpp"
#include "LoRa.hpp"
#include "SqlDb.hpp"

namespace LoRa
{
	class DatabaseCS : public LoRa::ApplicationDatabase//throws exception
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
				: SqlDb::ClientSingleQuery(myServer, "SELECT eui, appEui FROM motes", true)
			{}

			MoteRecord Read();	// returns an all ones address when no more to be read
		};

		DatabaseCS() {}

		uint64 AddApplicationData(EuiType moteEui, TimeRecord const& time, uint32 seqno, uint8 port, uint8 const data[], uint16 length);

		bool AddMoteTransmissionRecord(uint64 databaseId, uint32 frequency_Hz, LoRa::DataRate const& dataRate, LoRa::CodeRate const& codeRate, bool adrEnabled);

		bool AddGatewayReceptionRecord(uint64 databaseId, uint16 rank, EuiType gatewayEui,
			TimeRecord const& receiveTime, ValidValueUint16 const& channel, ValidValueUint16 const& rfChain, 
			ValidValueSint16 const& signalToNoiseRatio_cB, ValidValueSint16 const& signalStrength_cBm);
			//rank zero is the best reception record for a given frame, rank one is the second best and so on
		bool SetGatewayLastFrameId(EuiType gatewayEui, uint64 frameDatabaseId);

		bool CreateMote(EuiType moteEui, EuiType appEui);
		bool DeleteMote(EuiType moteEui);
		void DeleteAllMotes() {db.EmptyTable("motes");}
		bool SetMoteMostRecentlyReceivedFrame(EuiType moteEui, uint64 mostRecentlyReceivedFrameDatabaseId);

	private:
		bool ExtendAppData() {return db.Query("ALTER TABLE appdata CHANGE data data VARCHAR(500) CHARACTER SET armscii8 COLLATE armscii8_general_ci NOT NULL");}

		//Redefined virtual function
		bool UpdateDBSpecificStructure() {return ExtendAppData();}
	};
}

#endif

