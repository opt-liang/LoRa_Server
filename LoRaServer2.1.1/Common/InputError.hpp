/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef INPUT_ERROR
#define INPUT_ERROR

class InputError	//Exception
{
private:
	bool const			syntaxError;	// if false, the error is a parameter error (correct syntax but incorrect value)
	std::string			text;

public:
	InputError(bool mySyntaxError, std::string const& myText)
		: syntaxError(mySyntaxError), text(myText)
	{}

	InputError(bool mySyntaxError, std::stringstream const& myText)
		: syntaxError(mySyntaxError), text(myText.str())
	{}

	InputError(std::string const& myText)
		: syntaxError(true), text(myText)
	{}

	InputError(std::stringstream const& myText)
		: syntaxError(true), text(myText.str())
	{}

	std::string const& Text() const {return text;}
	bool SyntaxError() const {return syntaxError;}	//if false the error is a parameter error
};

#endif

