/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef SERVER_ADDRESS_CONTROLLER_HPP
#define SERVER_ADDRESS_CONTROLLER_HPP

#include "General.h"
#include "Ip.h"
#include "LoRa.hpp"
#include "LoRaIpPorts.hpp"
#include "Utilities.hpp"

class ServerAddressController
{
public:

private:
	sockaddr_in*		addressArray;
	uint				selected;	//when >= numberOfAddresses the value is invalid
	uint const			numberOfAddresses;

public:
	ServerAddressController(uint myNumberOfAddresses)
		:addressArray(new sockaddr_in[myNumberOfAddresses]), 
		selected(myNumberOfAddresses),
		numberOfAddresses(myNumberOfAddresses)
	{
		for (uint i = 0; i < numberOfAddresses; i++)
			SetInvalid(addressArray[i]);
	}

	virtual ~ServerAddressController()
	{
		delete [] addressArray;
	}

	sockaddr_in const& GetAddress(uint index) const
	{
		if (index >= numberOfAddresses)
			throw Exception(__LINE__, __FILE__, "Unexpected input to Address");

		return addressArray[index];
	}

	sockaddr_in const& GetAddress() const {return GetAddress(selected);}

	virtual bool SetAddress(uint index, sockaddr_in const& address)
	{
		if (index >= numberOfAddresses)
			throw Exception(__LINE__, __FILE__, "Unexpected input to Address");

		if (!IsValidPort(address))
			return false;

		addressArray[index] = address;

		if (!Selected())
			selected = index;

		return true;
	}

	bool Selected() const {return selected < numberOfAddresses;}

	bool Valid(uint index) const
	{
		sockaddr_in const& address = GetAddress(index);

		return IsValid(address);
	}

	bool Select(uint index)
	{
		if (!Valid(index))
			return false;

		selected = index;
		return true;
	}
};

#endif
