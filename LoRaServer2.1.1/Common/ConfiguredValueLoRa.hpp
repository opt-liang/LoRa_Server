/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef CONFIGURED_VALUE_LORA_HPP
#define CONFIGURED_VALUE_LORA_HPP

#include "ConfiguredValue.hpp"
#include "LoRa.hpp"

#include <string>

namespace LoRa
{
	typedef ConfiguredValueNonIntegerTemplate<LoRa::CypherKey> ConfiguredCypherKey;
}


#endif
