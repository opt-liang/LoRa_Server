/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "ConfiguredValueLoRa.hpp"
#include "Utilities.hpp"

template<> std::string ConfiguredValueNonIntegerTemplate<class LoRa::CypherKey>::ValueText(void) const
{
	std::string result = value.CastToString();

	return result;
}

template<> bool ConfiguredValueNonIntegerTemplate<class LoRa::CypherKey>::IsValid(std::string const& text) const
{
	LoRa::CypherKey key = text.c_str();

	return key.Valid();
}

template<> void ConfiguredValueNonIntegerTemplate<class LoRa::CypherKey>::ReadValue(std::string const& text)
{
	value = text.c_str();
}

