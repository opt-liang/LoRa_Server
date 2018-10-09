/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "CommandLineInterface.hpp"
#include "Utilities.hpp"
#include "JsonCommand.hpp"


CommandLineInterface::ClientList::~ClientList()
{
	//lint --e{1551} (Warning -- Function may throw exception '...' in destructor
	while (!empty())
		pop_front();
}


void CommandLineInterface::ClientList::Seen(MessageAddress const& address)
{
	std::list<CommandLineInterface::Client>::iterator it = begin();

	//lint -e{1702} Info -- operator 'operator!=' is both an ordinary function and a member function
	for (;it != end(); ++it)
	{
		if (it->Address() == address)
		{
			it->Seen();
			return;
		}
	}

	// not found;
	push_back(address);
}

void CommandLineInterface::ClientList::Tick()
{
	std::list<CommandLineInterface::Client>::iterator it = begin();

	//lint -e{1702} Info -- operator 'operator!=' is both an ordinary function and a member function
	while (it != end())
	{
		it->Tick();

		if (it->TicksSinceLastMessage() >= maxTicks)
		{
			std::list<CommandLineInterface::Client>::iterator deletedElement = it;

			it = erase(deletedElement);	// sets it to next element
		}
		else
			++it;
	}
}

void CommandLineInterface::Server::Write(char const text[], bool broadcast)
{
	std::string jsonString = JSON::GenerateCommand(text, false);

	if (broadcast)
	{
		std::list<CommandLineInterface::Client>::iterator it = clientList.begin();

		//lint -e{1702} Info -- operator 'operator!=' is both an ordinary function and a member function
		for (;it != clientList.end(); ++it)
			Send(it->Address(), jsonString);
	}
	else if (currentSource.Active())
		Send(currentSource.Address(), jsonString);
}

