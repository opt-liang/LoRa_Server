/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef CONFIGURED_VALUE_HPP
#define CONFIGURED_VALUE_HPP

#include "General.h"
#include "SqlDb.hpp"

#include <string>
#include <vector>

//ConfiguredValueBaseType holds the textual functions - no specialisation for the actual type of data held

class ConfiguredValueBaseType;


class ConfiguredValueBaseType
{
protected:
	class Initialiser	// this class is only instantiated by the constructor of ConfiguredValueBaseType and only deleted by its destructor
	{
		/* Only one instance is expected.  
		The object is not static because it MUST be created before any ConfiguredValueBaseType instances
		and not having it static allows its position within the initialisation sequence to be defined */

	private:
		std::vector<ConfiguredValueBaseType*> valueVector;
		static Initialiser*		initialiser;

	public:
		Initialiser()
		{}

		bool Empty() const {return valueVector.size() == 0;}
		void Initialise();

		static Initialiser& GetInitialiser()
		{
			if (!initialiser)
				initialiser = new Initialiser;
			return *initialiser;
		}

		static void Add(ConfiguredValueBaseType& value)
		{
			GetInitialiser().AddPrivate(value);
		}

		static void Remove(ConfiguredValueBaseType& value)
		{
			GetInitialiser().RemovePrivate(value);

			if (GetInitialiser().Empty())
			{
				delete initialiser;
				initialiser = 0;
			}
		}

	private:
		void AddPrivate(ConfiguredValueBaseType& value) {valueVector.push_back(&value);}

		void RemovePrivate(ConfiguredValueBaseType& value)
		{
			std::vector<ConfiguredValueBaseType*>::iterator it = valueVector.begin();

			for (;it != valueVector.end();++it)
			{
				if (&value == *it)
				{
					valueVector.erase(it);
					break;
				}
			}
		}
	};

protected:
	/*the database must hold a table called tableName.  The table must hold text columns named databaseNameColumn and databaseValueColumn */
	static SqlDb::Server&			database;
	std::string						name;

	ConfiguredValueBaseType(std::string const& myName)
		: name(myName)
	{Initialiser::Add(*this);}

	virtual ~ConfiguredValueBaseType() {Initialiser::Remove(*this);}

	std::string ReadFromDB();
	bool WriteToDB();	//returns false on failure

public:
	std::string const& Name() const {return name;}
	virtual std::string ValueText() const = 0;	//returns the object value as text
	virtual bool IsValid(std::string const& text) const = 0;	//true if the text if of the expected format
	bool IsValid(std::stringstream const& text) const {return IsValid(text.str());}	//true if the text if of the expected format
	void Initialise() {UpdateFromDB();}

	static void GlobalInitialise() {Initialiser::GetInitialiser().Initialise();}

protected:
	static char const* TableName() {return "configuration";}
	static char const* DatabaseNameColumn() {return "name";}
	static char const* DatabaseValueColumn() {return "value";}
	virtual bool UpdateFromDB() = 0;
};


//Specialisation template
template <typename T> class ConfiguredValueTemplate : public ConfiguredValueBaseType
{
protected:
	T		value;
public:
	ConfiguredValueTemplate(std::string const& myName, T const& defaultValue)
		: ConfiguredValueBaseType(myName), value(defaultValue)
	{
		/* ANY CHILD MUST call UpdateFromDB() in its constructor */
	}

	void Initialise() {UpdateFromDB();}	//redefined virtual function
	T const& Value() const {return value;}

	operator std::string () const {return ValueText();}
	operator T const& () const {return Value();}

	T operator=(std::string const& newValue)
	{
		if (IsValid(newValue))
		{
			ReadValue(newValue);
			WriteToDB();
		}
		return value;
	}

protected:
	bool UpdateFromDB()
	{
		std::stringstream text;
		
		text << ReadFromDB();

		bool result = IsValid(text.str());

		if (result)
			ReadValue(text);
		return result;
	}

	virtual bool IsValid(std::string const& text) const = 0;	//returns true if text is good

private:
	virtual void ReadValue(std::string const& text) = 0;	//text is known to be good
	void ReadValue(std::stringstream const& text) {ReadValue(text.str());}

};

//Simple specialisation of ConfiguredStoredValueTemplate allowing integer types to be generated easily
template <typename T> class ConfiguredValueIntegerTemplate : public ConfiguredValueTemplate<T>
{
protected:
	bool const	representAsHex;

public:
	ConfiguredValueIntegerTemplate(std::string const& myDatabaseName, T const& defaultValue, bool myRepresentAsHex = false)
		:ConfiguredValueTemplate<T>(myDatabaseName, defaultValue), representAsHex(myRepresentAsHex)
	{}

	//redefined virtual functions
	std::string ValueText() const
	{
		std::stringstream text;

		if (representAsHex)
			text << std::hex;

		text << ConfiguredValueTemplate<T>::value;

		return text.str();
	}

	bool IsValid(std::string const& text) const
	{
		std::stringstream s(text);

		T readValue;

		if (representAsHex)
			s << std::hex;
		
		return (s >> readValue) ? true : false;
	}

	void ReadValue(std::string const& text)
	{
		std::stringstream s(text);

		T readValue;
		
		if (representAsHex)
			s >> std::hex;

		s >> readValue;
		ConfiguredValueTemplate<T>::value = readValue;
	}

	T operator=(T const& newValue)
	{
		ConfiguredValueTemplate<T>::value = newValue;
		ConfiguredValueTemplate<T>::WriteToDB();
		return ConfiguredValueTemplate<T>::value;
	}

	T Read(std::string const& newValue)
	{
		if (IsValid(newValue))
		{
			ReadValue(newValue);
			ConfiguredValueTemplate<T>::WriteToDB();
		}
		return ConfiguredValueTemplate<T>::value;
	}
};

//Derived from ConfiguredValueTemplate allowing ConfiguredValueIntegerTemplate to define integer to text conversion functions in template
template <typename T> class ConfiguredValueNonIntegerTemplate : public ConfiguredValueTemplate<T>
{
public:
	ConfiguredValueNonIntegerTemplate(std::string const& myName, T const& defaultValue)
		:ConfiguredValueTemplate<T>(myName, defaultValue)
	{}

	std::string ValueText() const;

	T operator=(T const& newValue)
	{
		ConfiguredValueTemplate<T>::value = newValue;
		ConfiguredValueTemplate<T>::WriteToDB();
		return ConfiguredValueTemplate<T>::value;
	}

	T Read(std::string const& newValue)
	{
		ReadValue(newValue);
		ConfiguredValueTemplate<T>::WriteToDB();
		return ConfiguredValueTemplate<T>::value;
	}

private:
	bool IsValid(std::string const& text) const;
	void ReadValue(std::string const& text);
};


//Do not use single byte integers because write to text interprets them as characters
typedef ConfiguredValueIntegerTemplate<uint16>				ConfiguredUint16;
typedef ConfiguredValueIntegerTemplate<uint32>				ConfiguredUint32;
typedef ConfiguredValueIntegerTemplate<sint16>				ConfiguredSint16;
typedef ConfiguredValueNonIntegerTemplate<bool>				ConfiguredBool;
typedef ConfiguredValueNonIntegerTemplate<std::string>		ConfiguredString;


#endif

