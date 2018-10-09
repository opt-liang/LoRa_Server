/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "Application.hpp"
#include "LoRaApplicationDatabase.hpp"
#include "GlobalData.hpp"

void Application::Element::CreateNVRecord() const
{
	if (Id() != invalidEui) // don't write null application to database
		Global::genericDatabase.CreateApplication(Id(), Name(), Owner());
}


namespace BinarySearchVectorNV
{
	//lint -e{1762} Info -- Member function 'BinarySearchVectorNV::List<Application::Element,unsigned long long>::Initialise(void)' could be made const)
	template<> void List<class Application::Element, EuiType>::Initialise(void)
	{
		LoRa::ApplicationDatabase::ApplicationClient applicationClient(Global::genericDatabase);

		for (;;)
		{
			LoRa::ApplicationDatabase::ApplicationRecord applicationRecord = applicationClient.Read();

			if (applicationRecord.eui == invalidEui)
				break;

			Application::Element* app = new Application::Element(applicationRecord.eui, applicationRecord.name.c_str(), applicationRecord.owner.c_str());

			MutexHolder mutexHolder(mutex);
			memory.Add((BinarySearchVector::ElementTemplate<Application::Element, EuiType> *) app);
		}
	}

	template<> void DatabaseList<class Application::Element, EuiType>::Delete(EuiType eui) const
	{
		Global::genericDatabase.DeleteApplication(eui);
	}

	template<> void DatabaseList<class Application::Element,EuiType>::DeleteAllElements(void) const
	{
		Global::genericDatabase.DeleteAllApplications();
	}
}

void Application::ServerListNV::Initialise()
{
	LoRa::ApplicationDatabase::ServerClient serverClient(Global::genericDatabase, Application().Id());

	for (;;)
	{
		LoRa::ApplicationDatabase::ServerRecord serverRecord = serverClient.Read();

		if (!serverRecord.IsValid())
			break;

		Server* server = new Server(Application(), serverRecord.address, serverRecord.active, serverRecord.mask);

		memory.Add(*server);
	}
}


bool Application::Server::CreateNVRecord() const
{
	return Global::genericDatabase.CreateApplicationServer(Application().Id(), Address(), ActiveConnection(), ServiceMask());
}


bool Application::ServerListNV::SetServiceMaskNV(sockaddr_in const& address, Service::Mask serviceMask)
{
	return Global::genericDatabase.UpdateApplicationServer(Application().Id(), address, serviceMask);
}


void Application::ServerListNV::DeleteFromNVStore(sockaddr_in const& address)
{
	Global::genericDatabase.DeleteApplicationServer(memory.Application().Id(), address);
}

