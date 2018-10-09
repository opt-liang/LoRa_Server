/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef OPTION_CONTROLLER_HPP
#define OPTION_CONTROLLER_HPP

#include "LoRa.hpp"

class OptionController
{
private:
	bool					active;

protected:
	OptionController()
		: active(false)
	{}

	virtual ~OptionController() {}

	LoRa::OptionRecord		command;

public:
	bool Active() const {return active;}
	void Active(bool a) {active = a;}
	LoRa::OptionRecord const& GetOption() {active = false; return command;}
	uint16 OptionLength() const {return command.Length();}

	LoRa::OptionRecord const& GetCommand()
	{
		UpdateCommand();
		Active(false);
		return command;
	}

private:
	virtual void UpdateCommand() = 0;
};

#endif

