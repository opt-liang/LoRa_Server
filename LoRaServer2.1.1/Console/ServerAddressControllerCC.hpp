/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef SERVER_ADDRESS_CONTROLLERLT_HPP
#define SERVER_ADDRESS_CONTROLLERLT_HPP

#include "ServerAddressController.hpp"

class ServerAddressControllerCC : public ServerAddressController
{
public:
	enum AddressType {networkServer, applicationServer, customerServer, networkController, numberOfAddressTypes, unknown = numberOfAddressTypes};

	ServerAddressControllerCC()
		: ServerAddressController(numberOfAddressTypes)
	{}

	virtual ~ServerAddressControllerCC()
	{}

	static AddressType Read(std::string const& input)
	{
		if (input == "ns")			return networkServer;
		else if (input == "as")		return applicationServer;
		else if (input == "cs")		return customerServer;
		else if (input == "nc")		return networkController;
		else						return unknown;
	}

	bool SetAddress(uint type, sockaddr_in const& input)  	//redefined virtual function - uint type represents AddressType
	{
		sockaddr_in address = input;

		if (!IsValid(address))
			return false;

		if (!IsValidPort(address))
		{
			if (type != networkServer)
				return false;

			address.sin_port = htons(LoRa::UDP::jsonPort);	//default for network server
		}

		bool result = ServerAddressController::SetAddress(type, address);

		return result;
	}
};


#endif