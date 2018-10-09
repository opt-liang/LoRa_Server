/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef WORD_STORE_HPP
#define WORD_STORE_HPP

#include "General.h"
#include "Ip.h"
#include "LoRa.hpp"
#include "Utilities.hpp"

#include <string>
#include <vector>
#include <sstream>
#include <Eui.hpp>

class WordStore
{
private:
	uint16	currentWord;	//index of next word to read
	std::vector<std::string> word;	// holds the words of the command that have not be consumed
	std::string filename;
	unsigned lineCount;

public:
	WordStore() : currentWord(0), lineCount(0)
	{}

	WordStore(std::string const& line)
		: currentWord(0), lineCount(0)
	{
		Read(line);
	}

	WordStore(char const line[])
		: currentWord(0), lineCount(0)
	{
		Read(line);
	}
	virtual ~WordStore() {}

	void Read(std::string const& line) {Read(line.c_str());}
	void Read(char const line[]) {ConvertStringToWordVector(line, word);}

	uint16 WordsInStore() const {return word.size();}
	uint16 WordsRemaining() const {return WordsInStore() - currentWord;}
	bool NoWordsRemaining() const {return WordsRemaining() == 0;}

	void ReturnWord(uint words = 1)
	{
		if (currentWord >= words)
			currentWord -= words;
	}

	std::string const& GetNextWord();	//returns next word and removes it
	std::string const& NextWord() const; //returns next word without removing it.
	unsigned CurrentWord() const {return WordsInStore() - WordsRemaining();}


	//The following functions throw SyntaxError exceptions if the expected value is not present
	double GetNextDouble(); //returns next word (GetNextWord()) as a float or throws SyntaxError
	float GetNextFloat() {return static_cast<float>(GetNextDouble());} //returns next word (GetNextWord()) as a float or throws SyntaxError

	bool GetNextBoolean(bool allowBlank = false);
	//returns next word (GetNextWord()) as a bool or throws SyntaxError
	//if allowBlank is true and next word is blank, true is returned

	sint32 GetNextSigned();
	uint32 GetNextUnsigned(bool hex = false, bool separatorsAllowed = false);
	uint64 GetNextUnsignedLong(bool hex = false, bool separatorsAllowed = false);

	struct sockaddr_in GetNextIpAddress(bool throwException = true); //next word must be ip address or host name - optionally followed by ':' and port number
	// if throwException is true and the text is not a valid address format (x.x.x.x or x.x.x.x:nn) a SyntaxError exception is thrown

	struct sockaddr_in GetNextIpPortAddress(bool throwException = true); //next word must be ip address or host name - followed by ':' and port number
	// if throwException is true and the text is not a valid address format (x.x.x.x or x.x.x.x:nn) a SyntaxError exception is thrown

	LoRa::CypherKey GetNextCypherKey(bool throwException = true);
	EuiType GetNextEui() {return GetNextUnsignedLong(true, true);}
	uint32 GetNextNetworkAddress() {return GetNextUnsigned(true, true);}
	void PrintApplicationList(bool printServers) const;

	std::string ReadHexText();	
	//reads words until there are no more or until one is not a hex pair (e.g. 0a, 0A, 0, a, A)

	double GetNextCoordinate(char positiveSuffix, char negativeSuffix); //e.g. to read north positive latitude GetNextCoordinate('N', 'S');
	void EchoLine(char const line[]) const;

	static bool IsBooleanTrue(std::string const& input);
	static bool IsBooleanFalse(std::string const& input);
	bool IsBoolean(std::string const& input) const {return IsBooleanTrue(input) || IsBooleanFalse(input);}

	std::string FormRemainingWordsIntoString()
	{
		std::stringstream result;
		while (WordsRemaining())
			result << GetNextWord() << " ";
		return result.str();
	}

	class SyntaxError //exception class
	// WARNING exception MUST be caught while WordStore is still in scope
	{
	private:
		WordStore const& store;
		std::string explanation;

	public:
		SyntaxError(WordStore const& myStore, char const myExplanation[] = "") 
			: store(myStore), explanation(myExplanation) {}

		unsigned CurrentWord() const {return store.CurrentWord();}

		std::string const& Text() const {return explanation;}
	};
};


#endif
