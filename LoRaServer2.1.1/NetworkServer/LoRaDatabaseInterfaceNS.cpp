/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "LoRaDatabaseNS.hpp"
#include "GlobalDataNS.hpp"
#include "MoteNS.hpp"

void Mote::CreateNVRecord() const
{
	Global::networkDatabase.CreateMote(Id(), ClientApplicationEui(), NetworkAddress(), NetworkSessionKey(), 
		frameCopyArbitrator.LastUpMessageSequenceNumber(), transmitController.GetCurrentDownstreamSequenceNumber());
}

void Gateway::CreateNVRecord() const
{
	Global::networkDatabase.CreateGateway(Id(), region, allowGpsToSetPosition);
}

namespace BinarySearchVectorNV
{
	template<> void List<class Mote, EuiType>::Initialise(void)
	{
		LoRa::DatabaseNS::MoteClient client(Global::networkDatabase);

		for (;;)
		{
			LoRa::DatabaseNS::MoteRecord record = client.Read();

			if (record.moteEui == invalidEui)
				break;

			Global::joinController.ReserveNetworkAddress(record.moteEui, record.networkAddress);
			Mote* mote = new Mote(record.moteEui, record.appEui, record.networkAddress, record.networkSessionKey, record.downMessageSequenceNumber, record.upMessageSequenceNumber);

			MutexHolder holder(mutex);
			memory.Add((BinarySearchVector::ElementTemplate<Mote, uint64> *) mote);
		}
	}


	template<> void DatabaseList<class Mote, EuiType>::Delete(EuiType eui) const
	{
		Global::networkDatabase.DeleteMote(eui);
	}

	template<> void DatabaseList<class Mote,EuiType>::DeleteAllElements(void) const
	{
		Global::networkDatabase.DeleteAllMotes();
	}

	template<> void List<class Gateway, EuiType>::Initialise(void)
	{
		LoRa::DatabaseNS::GatewayClient client(Global::networkDatabase);

		for (;;)
		{
			LoRa::DatabaseNS::GatewayRecord record = client.Read();

			if (record.eui == invalidEui)
				break;

			if (record.region >= LoRa::numberOfRegions)
				record.region = Global::GetDefaultRegion();

			Gateway* gateway = new Gateway(record.eui, record.region, record.position, record.allowGpsToSetPosition);

			MutexHolder mutexHolder(mutex);
			memory.Add((BinarySearchVector::ElementTemplate<Gateway, uint64> *) gateway);
		}
	}

	template<> void DatabaseList<class Gateway, EuiType>::Delete(EuiType eui) const
	{
		Global::networkDatabase.DeleteGateway(eui);
	}


	template<> void DatabaseList<class Gateway,EuiType>::DeleteAllElements(void) const
	{
		Global::networkDatabase.DeleteAllGateways();
	}
}

