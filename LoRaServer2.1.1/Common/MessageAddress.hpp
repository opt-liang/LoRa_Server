/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef MESSAGE_ADDRESS_HPP
#define MESSAGE_ADDRESS_HPP

#include "Ip.h"
#include "TcpSocket.hpp"
#include "TcpConnectionManager.hpp"
#include "ValueWithValidity.hpp"
#include "Utilities.hpp"

#include <memory.h>
#include <string>
#include <sstream>

class MessageAddress
{
private:
	ValueWithValidity<TCP::Connection::IdType>		tcpConnectionId;
	sockaddr_in										address;

public:
	MessageAddress(sockaddr_in const& myAddress)
		: address(myAddress)
	{}

	MessageAddress()
	{SetInvalid(address);}

	MessageAddress(uint32 myTcpConnectionId)
		: tcpConnectionId(myTcpConnectionId)
	{SetInvalid(address);}

	bool ConnectionValid() const {return tcpConnectionId.Valid();}
	bool AddressValid() const {return ::IsValidPort(address);}

	//lint --e{1539}  (Info --Warning -- member 'MessageAddress::address' not assigned by assignment operator
	MessageAddress& operator=(uint32 other) {tcpConnectionId = other; return *this;}
	MessageAddress& operator=(sockaddr_in const& other) {address = other; return *this;}
	MessageAddress& operator=(MessageAddress const& other)
	{
		if (other.tcpConnectionId.Valid())
			tcpConnectionId = other.TcpConnectionId();

		if (::IsValidPort(other.Address()))
			address = other.Address(); 

		return *this;
	}
	MessageAddress& operator=(std::string const& text) {return (*this = text.c_str());}
	MessageAddress& operator=(char const text[])
	{
		IP::ReadAddressText(text, address);
		return *this;
	}
	

	ValidValueUint32 const& TcpConnectionId() const {return tcpConnectionId;}
	sockaddr_in const& Address() const {return address;}

	bool IsValid(bool useAnd = false) const
	{
		if (useAnd)
			return ConnectionValid() && AddressValid();
		else
			return ConnectionValid() || AddressValid();
	}

	bool IsLoopBack() const {return AddressValid() && ::IsLoopBack(Address());}

	std::string IPAddressText() const
	{
		std::stringstream text;

		if (AddressValid())
			text << ::AddressText(Address());
		
		return text.str();
	}

	std::string Text() const
	{
		std::stringstream text;

		if (AddressValid())
			text << ::AddressText(Address());
		else if (ConnectionValid())
			text << "via TCP connection " << uint32(tcpConnectionId);
		else
			text << "Invalid address";

		return text.str();
	}

	operator std::string() const {return Text();}
};

inline int operator==(MessageAddress const& l, MessageAddress const& r)
{
	if (l.AddressValid() != r.AddressValid()) return false;
	if (l.ConnectionValid() != r.ConnectionValid()) return false;

	if (l.AddressValid())
		if (l.Address() != r.Address()) return false;

	if (l.ConnectionValid())
		if (l.TcpConnectionId() != r.TcpConnectionId()) return false;

	return true;
}

#endif

