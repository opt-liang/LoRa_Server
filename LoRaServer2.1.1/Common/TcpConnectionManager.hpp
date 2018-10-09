/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef TCP_CONNECTION_MANAGER_HPP
#define TCP_CONNECTION_MANAGER_HPP

#include "TcpSocket.hpp"
#include "BinarySearchVector.hpp"
#include "General.h"
#include "IpSocketSet.hpp"

#include <vector>


namespace TCP
{
	class ConnectionManager;
	class Connection : public BinarySearchVector::ElementTemplate<Connection, uint32>	//must equal type of Connection::IdType
	{
	public:
		typedef uint32			IdType;	//uint32 is the 2nd base type is used in template definition - the two types must be the same
		static const IdType		invalidId = ~IdType(0);
		static const uint32		tickPeriod_ms = 250;

	private:
		ConnectionManager&		manager;
		TCP::Socket&			socket;	//socket is allocated by a new so must be deleted by the destructor
		uint16					clients;	//the number of clients using the connection
		sockaddr_in				advertisedPartnerAddress;
		bool const				initiateConnection;

	public:
		//mySocket MUST be allocated by the calling function using a new.  It is destroyed by this class's destructor
		Connection(ConnectionManager& myManager, TCP::Socket& mySocket, IdType id, bool myInitiateConnection)
			:BinarySearchVector::ElementTemplate<Connection, uint32>(id),
			manager(myManager), socket(mySocket), clients(0), initiateConnection(myInitiateConnection)
		{
			SetInvalid(advertisedPartnerAddress);
			socket.SetToBlock(false);
		}

		~Connection();

		TCP::Socket& Socket() const {return socket;}
		bool Open() {return socket.Open();} //Open must be called before connect
		bool Connect(sockaddr_in const& partnerAddress);
		void Disconnect();

		bool Connected() const {return socket.Connected();}
		bool AttemptingToConnect() const {return ActiveConnection() & !socket.Connected();}
		bool Blocking() const {return socket.Blocking();}
		bool PartnerAddressKnown() const {return IsValidPort(advertisedPartnerAddress);}
		bool ActiveConnection() const {return initiateConnection;}

		void AdvertisedPartnerPort(uint16 newPort);
		sockaddr_in const& AdvertisedPartnerAddress() const {return advertisedPartnerAddress;}
		sockaddr_in const& PartnerAddress() const
		{
			if (IsValidPort(AdvertisedPartnerAddress()))
				return AdvertisedPartnerAddress();
			else
				return socket.PartnerAddress();
		}

		sockaddr_in GetLocalAddress() const {return socket.GetLocalAddress();}

		void ClientAdded()	//must only be called when mutex is owned
		{
			clients++;
			CancelAnyPendingDeletionRequest();
		}

		void ClientRemoved();	//must only be called when mutex is owned
		uint16 Clients() const {return clients;}

		void VirtualTick();
	};

	class ConnectionManager : public BinarySearchVector::List<Connection, Connection::IdType>
	{
		friend class Connection;
		/*ConnectionManager holds pointers to internally created TCP sockets.  It assigns an ID to each.  
		This will not change and will not be reused (until 32 bit rollover), unlike 
		OS socket descriptors.  

		The ConnectionManager will delete its sockets when it exits

		An 'active' connection is one where this manager attempts to initiate a TCP connection to the remote port
		A 'passive' connection is one where this manager waits for the remote port to connect
		*/
	private:
		IP::SocketSet						socketSet;
		Connection::IdType					nextConnectionId;	// id of next connection to be created

	public:
		ConnectionManager(uint32 myListTickPeriod_ms, bool roundRobin)
			: BinarySearchVector::List<Connection, Connection::IdType>(myListTickPeriod_ms, Connection::tickPeriod_ms, BinarySearchVector::forever, true), // delete internally on timeout
			socketSet(roundRobin), nextConnectionId(0)
		{}

		virtual ~ConnectionManager();

		Connection::IdType RequestConnection(sockaddr_in const& partnerAddress, bool active);
		/*returns connection id.  
		
		If initiateConnection is true, initiates connection to remote port; otherwise waits for remote port to connect.  
		The value of initiateConnection is only significant for the first call for a partnerAddress; subsequest calls 
		*/
		Connection::IdType ReceivedConnection(SOCKET socketDescriptor, sockaddr_in const& remotePort);

		Connection* GetById(TCP::Connection::IdType id) const {return static_cast<Connection*>(BinarySearchVector::List<Connection, Connection::IdType>::GetById(id));}	//returned locked
		Connection* GetByIndex(BinarySearchVector::IndexType index) const {return static_cast<Connection*>(BinarySearchVector::List<Connection, Connection::IdType>::GetByIndex(index));}	//returned locked
		Connection* GetBySocket(TCP::Socket const& socket)	//returns Connection locked
		{
			MutexHolder holder(mutex);

			for (BinarySearchVector::IndexType i = 0; i < store.size(); i++)
			{
				Connection* ptr = (Connection*) store[i];

				if (&ptr->Socket() != &socket)
					continue;

				ptr->Lock();
				return ptr;
			}

			return 0;
		}

		sockaddr_in AdvertisedPartnerAddress(Connection::IdType id) const
		{
			Connection const* connection = GetById(id);

			if (!connection)
				return IP::nullAddress;

			sockaddr_in result = connection->AdvertisedPartnerAddress();
			connection->Unlock();
			return result;
		}

		sockaddr_in GetLocalAddress(Connection::IdType id) const
		{
			Connection const* connection = GetById(id);

			if (!connection)
				return IP::nullAddress;

			sockaddr_in result = connection->GetLocalAddress();
			connection->Unlock();
			return result;
		}

		Connection::IdType GetConnectionId(TCP::Socket const& socket);
		bool RemoveClient(Connection::IdType id);	//returns false if connection not found

		void Tick() {BinarySearchVector::List<Connection, Connection::IdType>::Tick();}

		::TCP::Socket* Wait() {return (::TCP::Socket*) socketSet.Wait();}
		void RefreshConnections();	//reconnect any broken connections

		bool Send(Connection::IdType connection, uint8 const data[], uint16 length) const;
		bool Send(Connection::IdType connection, std::string const& text) const {return Send(connection, reinterpret_cast<uint8 const*>(text.c_str()), static_cast<uint16>(text.length() + 1));}
		bool Send(uint8 const data[], uint16 length) const;	// to all connections
		bool Send(std::string const& text) const {return Send(reinterpret_cast<uint8 const*>(text.c_str()), static_cast<uint16>(text.length() + 1));}

		void AdvertisedPartnerPort(Connection::IdType connection, uint16 port);
		std::string Print() const;

	private:
		Connection* GetByPartnerAddress(sockaddr_in const& address) const;
		Connection::IdType GetConnectionId(sockaddr_in const& address);

		Connection* CreateConnection(sockaddr_in const& remotePort, bool active);			//Connection will never be null
		Connection* AcceptPassiveConnection(SOCKET socketDescriptor, sockaddr_in const& remotePort, bool remotePortIsStable);	//Used for incoming connections.  Connection will never be null
		virtual TCP::Socket* CreateSocket();	//creates a socket that is either TCP::Socket or a child. Return is never null
		virtual TCP::Socket* CreateSocket(SOCKET socketDescriptor, sockaddr_in const& remotePort);	//creates a connected socket that is either TCP::Socket or a child. Return is never null

		Connection::IdType GetNextConnectionId()
		{
			Connection::IdType result = nextConnectionId++;

			if (nextConnectionId == Connection::invalidId)
				nextConnectionId = 0;

			return result;
		}
		void RemoveAllConnections();
	};
}

namespace BinarySearchVector
{
	template<> inline void ElementTemplate<class TCP::Connection, unsigned int>::TemplatedElementTick(void)
	{
		static_cast<TCP::Connection*>(this)->VirtualTick();
	}
}


#endif

