/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

//Functions that must be defined but are null

#include "JsonReceive.hpp"

void JSON::Receive::MacCommandMessage(EuiType /*moteEui*/, ValidValueUint16 const& /*token*/, LoRa::OptionRecord /*command*/)
{
}

void JSON::Receive::JoinMessage(char const /*receivedText*/[])
{
}


