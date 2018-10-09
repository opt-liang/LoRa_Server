/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "LoRaDatabaseCS.hpp"
#include "GlobalDataCS.hpp"
#include "MoteCS.hpp"
#include "Application.hpp"

void Mote::CreateNVRecord() const
{
	Global::customerDatabase.CreateMote(Id(), ApplicationEui());
}


namespace BinarySearchVectorNV
{
	//lint -e{1762} Info -- Member function 'BinarySearchVectorNV::List<Application::Element,unsigned long long>::Initialise(void)' could be made const)
	template<> void List<class Mote, EuiType>::Initialise(void)
	{
		LoRa::DatabaseCS::MoteClient client(Global::customerDatabase);

		for (;;)
		{
			LoRa::DatabaseCS::MoteRecord fileRecord = client.Read();

			if (fileRecord.moteEui == invalidEui)
				break;

			Mote* mote = new Mote(fileRecord.moteEui, fileRecord.appEui);

			MutexHolder holder(mutex);
			memory.Add((BinarySearchVector::ElementTemplate<Mote, EuiType> *) mote);
		}
	}


	template<> void DatabaseList<class Mote,EuiType>::Delete(EuiType eui) const
	{
		Global::customerDatabase.DeleteMote(eui);
	}

	template<> void DatabaseList<class Mote,EuiType>::DeleteAllElements(void) const
	{
		Global::customerDatabase.DeleteAllMotes();
	}
}

