/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef PORT_ADDDRESS_CONTROLLER_HPP
#define PORT_ADDDRESS_CONTROLLER_HPP

#include "TcpConnectionManager.hpp"
#include "General.h"
#include "JsonString.hpp"
#include "ValueWithValidity.hpp"

namespace TCP
{
	class ConnectionAddressController
	{
	public:
		static const uint8 transmitPeriod_s = 5;

	private:
		TCP::ConnectionManager&		manager;
		ValidValueUint16			advertisedLocalPort;

	public:
		ConnectionAddressController(TCP::ConnectionManager& myManager)
			: manager(myManager)
		{}

		void SetAdvertisedLocalPort(uint16 newPort) {advertisedLocalPort = newPort;}
		ValidValueUint16 AdvertisedLocalPort() const {return advertisedLocalPort;}

		void NewConnectionReceived(TCP::Connection::IdType id) const
		{
			std::string jsonString = GenerateQuery();
			manager.Send(id,jsonString);
		}

		void QueryReceived(TCP::Connection::IdType id) const
		{
			std::string jsonString = GenerateResponse();
			manager.Send(id,jsonString);
		}

		void ResponseReceived(TCP::Connection::IdType id, uint16 partnerPort)
		{
			manager.AdvertisedPartnerPort(id, partnerPort);
		}

	private:
		std::string GenerateQuery() const
		{
			JSON::String jsonString;

			jsonString.Open();	//top
			jsonString.OpenStructuredObject("ip", false);
			jsonString.AddTextValue("whichport", "", false);
			jsonString.Close();	//ip
			jsonString.Close();	//top

			return jsonString;
		}
		std::string GenerateResponse() const
		{
			JSON::String jsonString;

			jsonString.Open();	//top
			jsonString.OpenStructuredObject("ip" ,false);
			jsonString.AddUnsignedValue("publishedport", advertisedLocalPort, false);
			jsonString.Close();	//ip
			jsonString.Close();	//top

			return jsonString;
		}
	};
}
#endif
