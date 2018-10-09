/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "WordStore.hpp"
#include "Utilities.hpp"
#include <iomanip>


std::string const& WordStore::NextWord() const
{
	const static std::string blankWord;

	if (currentWord < word.size())
		return word[currentWord];
	else
		return blankWord;
}


std::string const& WordStore::GetNextWord()
{
	std::string const& result = NextWord();

	if (currentWord < word.size())
		currentWord++;

	return result;
}


double WordStore::GetNextDouble()
{
	double result;
	
	std::stringstream text;
	text << GetNextWord();
	
	if (!(text >> result))
		throw SyntaxError(*this);

	return result;
}


bool WordStore::GetNextBoolean(bool allowBlank)
{
	std::string const& nextWord = GetNextWord();

	if (allowBlank && nextWord == "") return true;
	if (IsBooleanTrue(nextWord)) return true;
	if (IsBooleanFalse(nextWord)) return false;

	throw SyntaxError(*this);
}

sint32 WordStore::GetNextSigned()
{
	std::stringstream text(GetNextWord());

	sint32 readValue;
	if (!(text >> readValue))
		throw SyntaxError(*this);

	return readValue;
}


uint32 WordStore::GetNextUnsigned(bool hex, bool separatorsAllowed)
{
	std::string const& text = GetNextWord();

	uint32 readValue = ReadUnsignedInteger(text, hex, separatorsAllowed);

	if (readValue == ~uint32(0))
		throw SyntaxError(*this);

	return readValue;
}

uint64 WordStore::GetNextUnsignedLong(bool hex, bool separatorsAllowed)
{
	std::string const& text = GetNextWord();

	uint64 readValue = ReadUnsignedLongInteger(text, hex, separatorsAllowed);

	if (readValue == ~uint64(0))
		throw SyntaxError(*this);

	return readValue;
}


struct sockaddr_in WordStore::GetNextIpAddress(bool throwException)
{
	sockaddr_in result;

	std::string const& addressText = GetNextWord();

	if (addressText.empty())
	{
		if (throwException)
			throw SyntaxError(*this, "Unable to read IP address");
		else
		{
			SetInvalid(result);
			return result;
		}
	}

	result = ReadIpAddressOrHostName(addressText);

	if (throwException && !IsValid(result))
		throw SyntaxError(*this, "Unable to read IP address");

	return result;
}


struct sockaddr_in WordStore::GetNextIpPortAddress(bool throwException)
{
	sockaddr_in result = GetNextIpAddress(false);	// never throw an exception on IP address failure - so that error message always refers to inability to read port

	if (throwException && !IsValidPort(result))
		throw SyntaxError(*this, "Unable to read IP port address");

	return result;
}


LoRa::CypherKey WordStore::GetNextCypherKey(bool throwException)
{
	std::string word = GetNextWord();

	LoRa::CypherKey key;
	if (word.empty())
	{
		if (throwException)
			throw SyntaxError(*this, "Unable to read cypher key");
		else
			key.Invalidate();
	}
	else
		key = word.c_str();

	return key;
}


std::string WordStore::ReadHexText()
{
	std::stringstream result;

	for (;;)
	{
		std::string const& current = GetNextWord();

		if (current.empty())
			break;	//no more words

		if (current.length() > 2)
			goto finish;

		uint8 read = ReadHexDigit(current[0]);

		if (read > 0xF)
			goto finish;

		uint8 byte = read;

		if (current.length() > 1)
		{
			read = ReadHexDigit(current[1]);

			if (read > 0xF)
				goto finish;

			byte = (byte << 4) | read;
		}
		result << std::hex << std::setw(2) << std::setfill('0') << unsigned(byte);
	}

	return result.str();
	
finish:
	ReturnWord();
	return result.str();
}


double  WordStore::GetNextCoordinate(char positiveSuffix, char negativeSuffix)
{
	std::string word = GetNextWord();	//reference not used because value is altered
	ValidValueBool positive;

	if (word.empty())
		throw SyntaxError(*this,"Unable to read coordinate");

	if (tolower(word[word.length()-1]) == tolower(positiveSuffix))
		positive = true;
	else if (tolower(word[word.length()-1]) == tolower(negativeSuffix))
		positive = false;

	if (positive.Valid())
		word.erase(word.length()-1,1);
	else
		positive = true;


	std::stringstream text;
	text << word;
	
	double result;
	if (!(text >> result))
		throw SyntaxError(*this);

	return (positive.Value() ? 1 : -1) * result;
}


bool WordStore::IsBooleanTrue(std::string const& input)
{
	if (input == "on") return true;
	if (input == "1") return true;
	return false;
}

bool WordStore::IsBooleanFalse(std::string const& input)
{
	if (input == "off") return true;
	if (input == "0") return true;
	return false;
}


