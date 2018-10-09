/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "ApplicationServer.hpp"
#include "Application.hpp"
#include "GlobalData.hpp"
#include "JsonGenerate.hpp"

#include <iomanip>


Application::Server::Server(Element const& myApplication, sockaddr_in const& myAddress, bool myActiveConnection, Service::Mask myServiceMask)
	: address(myAddress), activeConnection(myActiveConnection), connection(TCP::Connection::invalidId), serviceMask(myServiceMask), application(&myApplication)
{
	connection = Global::tcpConnectionManager.RequestConnection(address, activeConnection);	// connection may be invalid
}


Application::Server::~Server()
{
	Global::tcpConnectionManager.RemoveClient(connection);
}


bool Application::Server::Send(std::string const& text) const
{
	if (connection == TCP::Connection::invalidId)
	{
		connection = Global::tcpConnectionManager.RequestConnection(address, activeConnection);

		if (connection == TCP::Connection::invalidId)
			return false;	//unable to send
	}
	return JSON::Send(connection, text);
}


Application::Server* Application::ServerList::GetByIndex(uint index) const
{
	if (index >= Size())
		return 0;

	return vector[index];
}


Application::Server* Application::ServerList::GetByAddress(sockaddr_in const& address) const
{
	for (uint i = 0; i < Size(); i++)
	{
		if (vector[i]->Address() == address)
			return vector[i];
	}
	return 0;
}

bool Application::ServerList::Add(Server& server)
{
	::List<Server*, sockaddr_in>::Add(&server, server.Address());

	return true;
}


bool Application::ServerList::Delete(sockaddr_in const& address)
{
	uint index = ::List<Server*, sockaddr_in>::FindIndex(address);

	if (index >= Size())
		return false;

	Server* server = vector[index];

	if (!server)
		throw Exception(__LINE__, __FILE__, "Unexpected null pointer");

	delete server;

	vector.erase(vector.begin() + index);

	return true;
}


bool Application::ServerList::SetServiceMask(sockaddr_in const& address, Service::Mask serviceMask)
{
	uint index = ::List<Server*, sockaddr_in>::FindIndex(address);

	if (index >= Size())
		return false;

	vector[index]->SetServiceMask(serviceMask);

	return true;
}


Service::Mask Application::ServerList::GetServiceMask(sockaddr_in const& address) const
{
	uint index = ::List<Server*, sockaddr_in>::FindIndex(address);

	if (index >= Size())
		return Service::nullMask;

	return GetServiceMask(index);
}


Service::Mask Application::ServerList::GetServiceMask(uint index)const
{
	if (index >= Size())
		return Service::nullMask;

	return vector[index]->ServiceMask();
}


sockaddr_in const& Application::ServerList::ServerAddress(uint index) const
{
	if (index >= Size())
		return IP::NullAddress();

	return vector[index]->Address();
}


bool Application::ServerList::Send(uint index, std::string const& text) const
{
	Server* server = GetByIndex(index);

	if (!server)
		return false;

	return server->Send(text);
}


void Application::ServerList::Print(std::stringstream& output) const
{
	for (uint i = 0; i < application.NumberOfServers(); i++)
		output << std::left << std::setw(nameWidth + spacerWidth) << AddressText(application.ServerAddress(i)) <<  spacer << Service::TextString(application.GetServiceMask(i)) << std::endl;
}


void Application::ServerListNV::Add(sockaddr_in const& address, bool active, Service::Mask serviceMask)
{
	Server* server = new Server(Application(), address, active, serviceMask);

	server->CreateNVRecord();
	memory.Add(*server);
}


bool Application::ServerListNV::Delete(sockaddr_in const& address)
{
	bool success = memory.Delete(address);

	DeleteFromNVStore(address);

	return success;
}

