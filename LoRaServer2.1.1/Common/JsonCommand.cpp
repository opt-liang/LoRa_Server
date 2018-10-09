/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "JsonCommand.hpp"
#include "JsonString.hpp"

std::string JSON::GenerateCommand(char const text[], bool requestAcknowledge, uint16 sequenceNumber)
{
	JSON::String output;

	output.Open();	//top
	
	if (requestAcknowledge)
		output.AddAcknowledgementRequest(sequenceNumber, false);

	output.AddTextValue("command", text, requestAcknowledge);	//only add comma if not first element

	output.Close();	//top

	return output;
}

std::string JSON::GenerateAcknowledgementMessage(uint16 number)
{
	JSON::String output;

	output.Open();
	output.AddAcknowledgement(number, false);
	output.Close();

	return output;
}


std::string JSON::ReceiveAcknowledgementRequest(char const receivedText[])
{
	ValidValueUint16 acknowledgeNumber = ReadUnsignedInteger(receivedText);
	std::string result;

	if (acknowledgeNumber.Valid())
		result = JSON::GenerateAcknowledgementMessage(acknowledgeNumber);
	
	return result;
}

