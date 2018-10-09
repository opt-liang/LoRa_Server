/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef APPLICATION_SERVER_HPP
#define APPLICATION_SERVER_HPP

#include "Service.hpp"
#include "List.hpp"

#include "Ip.h"
#include "TcpConnectionManager.hpp"

namespace Application
{
	class Element;
	class ServerListNV;

	class Server
	{
		friend class ServerListNV;
	private:
		sockaddr_in							address;
		bool								activeConnection;	//if true, initiates connection.  If false, waits for other end to connect
		mutable TCP::Connection::IdType		connection;			//idenity - never reused
		Service::Mask						serviceMask;	//one bit for each information type
		Element const*						application;	//never null but not a reference to allow operation=() to work

	public:
		Server(Element const& myApplication, sockaddr_in const& myAddress, bool myActiveConnection, Service::Mask myServiceMask);
		~Server();

		sockaddr_in const& Address() const {return address;}
		Element const& Application() const {return *application;}
		TCP::Connection::IdType Connection() const {return connection;}
		bool ActiveConnection() const {return activeConnection;}
		Service::Mask ServiceMask() const {return serviceMask;}
		void SetServiceMask(Service::Mask newServiceMask) {serviceMask = newServiceMask;}

		bool Send(std::string const& text) const;

	private:
		bool CreateNVRecord() const;	//called by ServerListNV
	};

	class ServerList : public ::List<Server*, sockaddr_in>
	{
	private:
		Element& application;

	public:
		ServerList(Element& myApplication)
			: application(myApplication)
		{}

		bool Add(Server& server);
		bool Delete(sockaddr_in const& address);

		Server* GetByIndex(uint i) const;
		Server* GetByAddress(sockaddr_in const& address) const;	//returns null on failure
		bool SetServiceMask(sockaddr_in const& address, Service::Mask serviceMask);
		Service::Mask GetServiceMask(sockaddr_in const& address) const;
		Service::Mask GetServiceMask(uint index) const;
		sockaddr_in const& ServerAddress(uint index) const;

		Element& Application() {return application;}

		bool Send(uint index, std::string const& text) const;

		void Print(std::stringstream& output) const;
	};

	class ServerListNV
	{
	private:
		ServerList	memory;

	public:

		ServerListNV(Element& myApplication, bool initialise = true)
			: memory(myApplication)
		{
			if (initialise)
				Initialise();
		}

		Element& Application() {return memory.Application();}

		void Add(sockaddr_in const& address, bool active, Service::Mask serviceMask);
		bool Delete(sockaddr_in const& address);

		Service::Mask GetServiceMask(sockaddr_in const& address) const {return memory.GetServiceMask(address);}
		Service::Mask GetServiceMask(uint index) const {return memory.GetServiceMask(index);}
		sockaddr_in const& ServerAddress(uint index) const {return memory.ServerAddress(index);}
		bool SetServiceMask(sockaddr_in const& address, Service::Mask serviceMask)
		{
			if (!memory.SetServiceMask(address, serviceMask))
				return false;
			
			return SetServiceMaskNV(address, serviceMask);
		}
		Server* GetByIndex(uint i) {return memory.GetByIndex(i);}
		Server* GetByAddress(sockaddr_in const& address) {return memory.GetByAddress(address);}
		uint Size() const {return memory.Size();}

		bool Send(uint index, std::string const& text) const {return memory.Send(index, text);}
		void Initialise();
		void Print(std::stringstream& output) const {memory.Print(output);}

	private:
		bool SetServiceMaskNV(sockaddr_in const& address, Service::Mask serviceMask);
		void DeleteFromNVStore(sockaddr_in const& address);
	};
}

#endif
