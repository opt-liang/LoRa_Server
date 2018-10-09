/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "TcpConnectionManager.hpp"
#include "DebugMonitor.hpp"
#include <iomanip>
#include "ExceptionClass.hpp"


TCP::Connection::~Connection()
{
	if (Debug::Print(Debug::verbose))
	{
		std::stringstream text;
		text << "Deleting connection " << Id() << " " << (initiateConnection ? "active" : "passive");
		Debug::Write(text);
	}

	if (Connected())
		Disconnect();
}


TCP::ConnectionManager::~ConnectionManager()
{
	RemoveAllConnections();
}


void TCP::ConnectionManager::RemoveAllConnections()
{
	while (!IsEmpty())
		DeleteByIndex(0);
}


void TCP::ConnectionManager::RefreshConnections()
{
	for (BinarySearchVector::IndexType i = 0;; i++)	//for loop has no termination condition
	{
		Connection* connection = GetByIndex(i);	//connection is locked

		if (!connection)
			return;	// end of loop

		if (!connection->AttemptingToConnect())
		{
			connection->Unlock();
			continue;
		}

		SOCKET sd = connection->Socket().SocketDescriptor();
		sockaddr_in partnerAddress = connection->PartnerAddress();
		TCP::Connection::IdType id = connection->Id();

		sockaddr_in localInterfaceAddress = connection->Socket().InterfaceAddress();
		uint16 localPortNumber = connection->Socket().GetPortNumber();
		connection->Socket().SetActiveInSocketSet(false);	//remove from socket set - in case it is still active

		//Unlock because even a non blocking connection request can wait for many milliseconds
		connection->Unlock();
		connection = 0;	//pointer may become invalid during the unlocked section

//Connection unlocked section
//*******************************************************************
		if (sd == INVALID_SOCKET)
			sd = IP::Socket::Reopen(IP::Socket::tcp, localInterfaceAddress, localPortNumber);

		bool connected = false;
		if (sd != INVALID_SOCKET)
			connected = TCP::Socket::Connect(sd, partnerAddress);
		else
		{
			if (Debug::Print(Debug::major))
				Debug::Write("TCP::ConnectionManager::RefreshConnections() unable to reopen socket");
			continue;
		}
//*******************************************************************
//Lock again

		connection = GetById(id);
		if (connection)
		{
			if (connected)
			{
				connection->Socket().SetSocketDescriptor(sd);
				connection->Socket().IsNowConnected(true, socketSet);

				if (Debug::Print(Debug::monitor))
				{
					std::stringstream text;
					text << "Now connected to IP host " << AddressText(connection->Socket().PartnerAddress()) << " via Connection " << connection->Id();
					Debug::Write(text);
				}
			}
			connection->Unlock();
		}
		else
		{
			//connection has been deleted while this thread gave up the mutex.
			if (Debug::Print(Debug::monitor))
				Debug::Write("Closing TCP socket - connection deleted");

			IP::Socket::Close(sd);
		}
	}
}


void TCP::ConnectionManager::AdvertisedPartnerPort(Connection::IdType connectionId, uint16 partnerPort)
{
	Connection* connection = GetById(connectionId);	//connection is locked

	if (!connection)
		return;

	connection->AdvertisedPartnerPort(partnerPort);
	connection->Unlock();
}


TCP::Connection* TCP::ConnectionManager::GetByPartnerAddress(sockaddr_in const& address) const
{
	MutexHolder holder(mutex);
	Connection* connection = 0;

	for (BinarySearchVector::IndexType i = 0; i < Size(); i++)
	{
		connection = (Connection*) store[i];

		if (!connection->PartnerAddressKnown())
			continue;

		if (connection->AdvertisedPartnerAddress() != address)
			continue;

		connection->Lock();
		return connection;
	}

	return 0;
}

TCP::Connection::IdType TCP::ConnectionManager::GetConnectionId(TCP::Socket const& socket)
{
	Connection* connection = GetBySocket(socket);	//connection is locked

	if (!connection)
		return Connection::invalidId;

	Connection::IdType result = connection->Id();

	connection->Unlock();
	return result;
}

TCP::Connection::IdType TCP::ConnectionManager::GetConnectionId(sockaddr_in const& address)
{
	Connection* connection = GetByPartnerAddress(address);

	if (!connection)
		return Connection::invalidId;

	Connection::IdType result = connection->Id();

	connection->Unlock();
	return result;
}


TCP::Connection::IdType  TCP::ConnectionManager::RequestConnection(sockaddr_in const& partnerAddress, bool active)
{
	Connection* connection = GetByPartnerAddress(partnerAddress);	//connection is locked if it is not NULL
	Connection::IdType id;

	if (Debug::Print(Debug::monitor))
	{
		std::stringstream text;

		text << "Connection to " << AddressText(partnerAddress) << " requested";

		if (connection)
			text << " Connection already exists";

		Debug::Write(text);
	}

	if (connection)
	{
		id = connection->Id();
	}
	else if (active)
	{
		connection = CreateConnection(partnerAddress, active);
		id = connection->Id();

		connection->Lock();
		Add(connection);
	}
	else
		return Connection::invalidId;	// do not create a connection - client will have to wait for one to be created

	connection->ClientAdded();
	connection->Unlock();

	return id;
}

TCP::Connection::IdType TCP::ConnectionManager::ReceivedConnection(SOCKET socketDescriptor, sockaddr_in const& remotePort)
{
	if (Debug::Print(Debug::monitor))
	{
		std::stringstream text;

		text << "Connection from " << AddressText(remotePort) << " received";

		Debug::Write(text);
	}

	Connection* connection = AcceptPassiveConnection(socketDescriptor, remotePort, false);

	Connection::IdType id = connection->Id();
	Add(connection);

	return id;
}


TCP::Socket* TCP::ConnectionManager::CreateSocket()
{
	//default virtual function
	return new TCP::Socket();
}

TCP::Socket* TCP::ConnectionManager::CreateSocket(SOCKET socketDescriptor, sockaddr_in const& remotePort)
{
	return new TCP::Socket(socketDescriptor, remotePort);
}


TCP::Connection* TCP::ConnectionManager::CreateConnection(sockaddr_in const& remotePort, bool active)
{
	if (Debug::Print(Debug::monitor))
	{
		std::stringstream text;

		text << "Creating " << (active?"active ":"passive ") << "connection - " << nextConnectionId << " to port " << AddressText(remotePort);

		Debug::Write(text);
	}

	TCP::Socket* socket = CreateSocket();	//virtual function;

	Connection* connection = new Connection(*this, *socket, GetNextConnectionId(), active);

	if (active)
		connection->Connect(remotePort);

	return connection;
}

TCP::Connection* TCP::ConnectionManager::AcceptPassiveConnection(SOCKET socketDescriptor, sockaddr_in const& remotePort, bool remotePortIsStable)
{
	if (Debug::Print(Debug::monitor))
	{
		std::stringstream text;

		text << "Accepting passive connection " << nextConnectionId << " to " << AddressText(remotePort);

		Debug::Write(text);
	}

	TCP::Socket* socket = CreateSocket(socketDescriptor, remotePort);	//virtual function;

	Connection* connection = new Connection(*this, *socket, GetNextConnectionId(), false);

	if (remotePortIsStable)
		connection->AdvertisedPartnerPort(ntohs(remotePort.sin_port));

	return connection;
}

bool TCP::ConnectionManager::RemoveClient(TCP::Connection::IdType id)
{
	TCP::Connection* connection = static_cast<TCP::Connection*>(GetById(id)); // connection is locked

	if (!connection)
		return false;

	connection->ClientRemoved();	// if clients reduced to zero, the connection requests its deletion

	connection->Unlock();

	return true;
}


bool TCP::Connection::Connect(sockaddr_in const& partnerAddress)
{
	advertisedPartnerAddress = partnerAddress;
	if (socket.Connected())
		return false;

	if (!socket.Open())
		return false;

	socket.Connect(false, partnerAddress);

	if (Debug::Print(Debug::monitor))
	{
		std::stringstream text;
		
		text << "TCP Manager attempting connection to " << AddressText(partnerAddress);

		Debug::Write (text);
	}
	return true;
}


void TCP::Connection::Disconnect()
{
	if (Debug::Print(Debug::monitor))
	{
		std::stringstream text;

		text << "Disconnecting connection " << Id() << " from " << AddressText(PartnerAddress()) << " - " << (clients - 1) << " clients";

		Debug::Write(text);
	}

	socket.Disconnect();
}


void TCP::Connection::ClientRemoved()
{
	if (Debug::Print(Debug::monitor))
	{
		std::stringstream text;

		text << "Removing client from connection " << Id() << " to " << AddressText(PartnerAddress()) << " - " << (clients - 1) << " remaining clients";

		Debug::Write(text);
	}

	if (clients > 0)
		clients--;

	if (clients == 0 && ActiveConnection())
		RequestDeletion();
}


void TCP::Connection::AdvertisedPartnerPort(uint16 newPort)
{
	advertisedPartnerAddress = socket.PartnerAddress();
	advertisedPartnerAddress.sin_port = htons(newPort);
	if (Debug::Print(Debug::monitor))
	{
		std::stringstream text;

		text << "Connection " << Id() << " setting partner address to " << AddressText(AdvertisedPartnerAddress());

		Debug::Write(text);
	}
}


void TCP::Connection::VirtualTick()
{
	if (Connected())
	{
		socket.SendKeepAliveData();

		if (!socket.InSocketSet())
		{
			if (Debug::Print(Debug::verbose))
			{
				std::stringstream text;

				text << "Adding connection " << Id() << " to " << AddressText(AdvertisedPartnerAddress()) << " to socket set";

				Debug::Write(text);
			}

			socket.AddToSocketSet(manager.socketSet);
		}
	}
	else if (!ActiveConnection())
	{
		//Not active and not connected
		if (Debug::Print(Debug::monitor))
		{
			std::stringstream text;

			text << "Deleting passive connection " << Id();
			Debug::Write(text);
		}
		RequestDeletion();
	}
}

bool TCP::ConnectionManager::Send(Connection::IdType connectionId, uint8 const data[], uint16 length) const
{
	Connection* connection = static_cast<Connection*>(GetById(connectionId));

	if (!connection)
		return false;

	bool success = connection->Connected();

	if (success)
		success = connection->Socket().Send(data, length);

	connection->Unlock();

	return success;
}

bool TCP::ConnectionManager::Send(uint8 const data[], uint16 length) const
{
	MutexHolder holder(mutex);
	bool success = true;

	for (BinarySearchVector::IndexType i = 0; i < Size(); i++)
	{
		Connection* connection = (Connection*) GetByIndexPrivate(i);

		if (!connection->Socket().Send(data, length))
			success = false;

		connection->Unlock();
	}

	return success;
}


std::string TCP::ConnectionManager::Print() const
{
	MutexHolder holder(mutex);

	uint16 connectedCount = 0;
	uint16 inSocketSetCount = 0;

	for (BinarySearchVector::IndexType i = 0; i < Size(); i++)
	{
		Connection* connection = (Connection*) store[i];

		if (connection->Connected())
			connectedCount++;

		if (connection->Socket().InSocketSet())
			inSocketSetCount++;
	}
	std::stringstream output;

	output <<
		std::setw(3) << Size() << " connections" << std::endl <<
		std::setw(3) << connectedCount << " active connections " << std::endl <<
		std::setw(3) << inSocketSetCount << " in socket set " << std::endl <<
		std::endl;

	for (BinarySearchVector::IndexType i = 0; i < Size(); i++)
	{
		Connection* connection = (Connection*) store[i];

		output << "Id " << i << " (SD " <<std::setw(4);

		if (connection->Socket().SocketDescriptor() != INVALID_SOCKET)
			output << connection->Socket().SocketDescriptor();
		else
			output << "Inv";
		
		output << ") " << 
			(connection->ActiveConnection() ? "A" : "P") <<

			" Port " << std::setw(5) << connection->Socket().GetPortNumber() << " ";
		
		if (connection->Socket().Connected())
			output << " C ";
		else
			output << "NC ";
		
		output << "Ptnr " << std::setw(16) << AddressText(connection->AdvertisedPartnerAddress());
		
		if (connection->AdvertisedPartnerAddress() != connection->Socket().PartnerAddress())
			output << " (" << std::setw(16) << AddressText(connection->Socket().PartnerAddress()) << ")";
		
		if (connection->Socket().InSocketSet())
			output << " In set ";
		
		output << std::endl;
	}

	return output.str();
}

