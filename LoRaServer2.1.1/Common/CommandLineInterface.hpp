/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef COMMAND_LINE_INTERFACE_HPP
#define COMMAND_LINE_INTERFACE_HPP

#include "General.h"
#include "UdpSocket.hpp"
#include "Mutex.hpp"
#include "MessageAddress.hpp"

#include <string>
#include <sstream>
#include <iostream>
#include <memory.h>
#include <list>

namespace CommandLineInterface
{
	uint16 const maxReceivedBytes = 2000;

	class Client
	{
	public:
		
	private:
		MessageAddress address;
		uint32 ticksSinceLastMessage;

	public:
		Client(MessageAddress const& myAddress)
			: address(myAddress), ticksSinceLastMessage(0)
		{}

		void Seen()	{ticksSinceLastMessage = 0;}

		void Tick() {ticksSinceLastMessage++;}

		uint32 TicksSinceLastMessage() const {return ticksSinceLastMessage;}
		MessageAddress const& Address() const {return address;}
	};

	class ClientList : public std::list<Client>
	{
	private:
		const uint32 maxTicks;

	public:
		ClientList(uint32 myMaxTicks) 
			: maxTicks(myMaxTicks)
		{}

		//lint -e{1509} (Warning -- base class destructor for class 'list' is not virtual)
		virtual ~ClientList();

		void Seen(MessageAddress const& address);
		void Tick();
	};

	class Server
	{
	private:
		class CurrentSource
		{
		private:
			bool				active;
			MessageAddress		address;
		public:
			CurrentSource()
				:active(false)
			{}

			void CommandReceived(MessageAddress const& source) 
				{address = source; active = true;}

			void CommandComplete() {active = false;}
			bool Active() const {return active;}
			MessageAddress const& Address() const {return address;}
		};

		ClientList		clientList;
		CurrentSource	currentSource;
		UDP::Socket&	socket;

	public:
		Server(UDP::Socket& mySocket, uint32 myMaxTicks)
			:clientList(myMaxTicks), socket(mySocket)
		{}

		void RequestReceived(MessageAddress const& source)
		{
			clientList.Seen(source);
			currentSource.CommandReceived(source);
		}

		void Write(std::string const& text) {Write(text.c_str(), false);}
		void Write(std::stringstream const& text) {Write(text.str().c_str(), false);}
		void Write(char const text[]) {Write(text, false);}

		void Broadcast(std::string const& text) {Write(text.c_str(), true);}
		void Broadcast(std::stringstream const& text) {Write(text.str().c_str(), true);}
		void Broadcast(char const text[]) {Write(text, true);}

	private:
		void Write(char const text[], bool broadcast);
	};

	bool Send(MessageAddress const& destination, char const data[], uint16 length);	//must be defined elsewhere
	inline bool Send(MessageAddress const& destination, std::string const& text) {return Send(destination, text.c_str(), (uint16) text.length() + 1);}
}


#endif
