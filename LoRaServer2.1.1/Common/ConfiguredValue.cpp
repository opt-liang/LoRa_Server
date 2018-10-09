/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "ConfiguredValue.hpp"

#include <sstream>

ConfiguredValueBaseType::Initialiser* ConfiguredValueBaseType::Initialiser::initialiser = 0;


void ConfiguredValueBaseType::Initialiser::Initialise()
{
	std::vector<ConfiguredValueBaseType*>::iterator it = valueVector.begin();

	for (;it != valueVector.end();++it)
	{
		(*it)->Initialise();
	}
}

std::string ConfiguredValueBaseType::ReadFromDB()
{
	std::stringstream query;
	query << "SELECT " << DatabaseValueColumn() << " FROM " << TableName() << " WHERE " << DatabaseNameColumn() << " = \"" << Name() << '\"';

	SqlDb::Client client(database);
	try
	{
		client.Query(query, true);
	}
	catch (SqlDb::Exception const&)
	{
		return "";
	}

	if (!client.GetNextRow())
		return "";

	std::string value = client.GetFieldValue(0);
	return value;
}

bool ConfiguredValueBaseType::WriteToDB()
{
	std::stringstream query;

	query << "REPLACE INTO " << TableName() << " SET " << DatabaseNameColumn() << " = \"" << Name() << "\", " << DatabaseValueColumn() << " = \"" << ValueText() << "\"";

	return database.Query(query);
}


template<> std::string ConfiguredValueNonIntegerTemplate<bool>::ValueText() const
{
	return value ? "1" : "0";
}

template<> bool ConfiguredValueNonIntegerTemplate<bool>::IsValid(std::string const& text) const
{
	if (text == "on") return true;
	else if (text == "1") return true;
	else if (text == "off") return true;
	else if (text == "0") return true;
	return false;
}


template<> void ConfiguredValueNonIntegerTemplate<bool>::ReadValue(std::string const& text)
{
	if (text == "on") value = true;
	else if (text == "1") value = true;
	else if (text == "off") value = false;
	else if (text == "0") value = false;
	return;
}


template<> std::string ConfiguredValueNonIntegerTemplate<std::string>::ValueText() const
{
	return value;
}

template<> bool ConfiguredValueNonIntegerTemplate<std::string>::IsValid(std::string const& text) const
{
	return (!text.empty()) ? true : false;
}


template<> void ConfiguredValueNonIntegerTemplate<std::string>::ReadValue(std::string const& text)
{
	value = text;
}


