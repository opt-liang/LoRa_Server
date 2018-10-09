/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "General.h"
#include "Eui.hpp"
#include "BinarySearchVectorNV.hpp"
#include "Utilities.hpp"
#include "Ip.h"
#include "MessageAddress.hpp"
#include "LoRa.hpp"
#include "TransmissionRecord.hpp"
#include "TcpConnectionManager.hpp"
#include "ApplicationServer.hpp"

#include <vector>
#include <string>
#include <sstream>

namespace Application
{
	class Element;

	//Constant defines
	std::string const spacer = "   ";
	uint const spacerWidth = spacer.length();
	const uint nameWidth = 20;
	const EuiType nullAppEui = invalidEui - 1;
	std::string const nullAppName = "null";

	class Element : public BinarySearchVector::ElementTemplate<Element, EuiType>
	{
	protected:
		std::string					name;
		std::string					owner;
		ServerListNV				servers;

	public:
		Element(EuiType myEui, std::string const& myName, std::string const& myOwner) 
			: BinarySearchVector::ElementTemplate<Element, EuiType>(myEui),
			name(myName), owner(myOwner), servers(*this, true)	// initialise servers immediately
		{}

		void Initialise() {servers.Initialise();}

		virtual ~Element() {}

		std::string const& Name() const {return name;}
		std::string const& Owner() const {return owner;}
		uint NumberOfServers() const {return servers.Size();}

		void SendQueueLengthToUpstreamServer(bool app, uint16 length);

		void Send(MessageAddress destination, std::string const& text) const;

		void Send(EuiType moteEui, bool up, ValidValueUint32 const& sequenceNumber, ValidValueUint16 const& token, 
			LoRa::FrameApplicationData const& payload, MoteTransmitRecord const& transmitRecord, GatewayReceiveList const& gatewayReceiveList) const;

		void Send(EuiType moteEui, bool up, ValidValueUint32 const& sequenceNumber, 
			LoRa::FrameApplicationData const& payload, MoteTransmitRecord const& transmitRecord, GatewayReceiveList const& gatewayReceiveList) const
		{Send(moteEui, up, sequenceNumber, invalidValueUint16, payload, transmitRecord, gatewayReceiveList);}

		void Send(EuiType moteEui, bool up, ValidValueUint32 const& sequenceNumber, ValidValueUint16 const& token, LoRa::FrameApplicationData const& payload) const
		{
			MoteTransmitRecord const transmitRecord;
			GatewayReceiveList const gatewayReceiveList;
			Send(moteEui, up, sequenceNumber, token, payload, transmitRecord, gatewayReceiveList);
		}
		void Send(Service::Mask serviceMask, std::string const& text);

		Service::Mask GetServiceMask(uint32 index) const {return servers.GetServiceMask(index);}
		Service::Mask GetServiceMask(sockaddr_in const& address) const {return servers.GetServiceMask(address);}
		bool SetServiceMask(sockaddr_in const& address, Service::Mask serviceMask) {return servers.SetServiceMask(address, serviceMask);}
		sockaddr_in const& ServerAddress(uint index) const {return servers.ServerAddress(index);}
		void AddServer(sockaddr_in const& address, bool active, Service::Mask serviceMask) {servers.Add(address, active, serviceMask);}
		bool DeleteServer(sockaddr_in const& address) {return servers.Delete(address);}

		void SendQueueLength(EuiType moteEui, uint16 length);
		void RequestSequenceNumberGrant(EuiType moteEui);
		void InformUpstreamServerOfMoteReset(EuiType moteEui);
		void InformUpstreamServerOfTransmissionToMote(EuiType moteEui, bool app, uint16 token);
		void InformUpstreamServerOfAcknowledgementFromMote(EuiType moteEui, uint16 token);

		void PrintServerList(std::stringstream& output) const {servers.Print(output);}

		void CreateNVRecord() const;
	};

	class List : public BinarySearchVectorNV::List<Element, EuiType>
	{
	public:
		List(bool initialise) : BinarySearchVectorNV::List<Element, EuiType>(initialise)
		{}

		void Initialise();

		EuiType GetEui(std::string const& name, std::string const& owner) const;

		bool Exists(EuiType eui) const {return BinarySearchVectorNV::List<Element, EuiType>::Exists(eui);}
		bool Exists(std::string const& name, std::string const& owner) const;
		Element* GetById(EuiType eui) const {return static_cast<Element*>(BinarySearchVectorNV::List<Element, EuiType>::GetById(eui));} 	//Gateway is locked
		Element* GetByIndex(uint32 index) const {return static_cast<Element*>(BinarySearchVectorNV::List<Element, EuiType>::GetByIndex(index));} 	//Gateway is locked
		Element* GetByName(std::string const& name, std::string const& owner) const;	//Element is returned locked

		bool AddServer(EuiType eui, sockaddr_in const& address, bool activeConnection, Service::Mask serviceMask);
		bool DeleteServer(EuiType eui, sockaddr_in const& address);

		bool Send(EuiType eui, Service::Mask serviceMask, std::string const& text);

		bool Add(EuiType eui, std::string const& name, std::string const& owner);

	protected:
		void Add(Element const* element) {BinarySearchVectorNV::List<Element, EuiType>::Add(element);}
	};

	inline int operator==(Server const& l, Server const& r) {return (l.Address() != r.Address()) ? true : false;}
}


//Required by List
inline bool Equals(Application::Server* const& element, sockaddr_in const& address)
{
	return (element->Address() == address) ? true : false;
}

#endif

