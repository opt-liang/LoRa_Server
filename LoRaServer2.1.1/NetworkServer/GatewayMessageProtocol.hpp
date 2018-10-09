/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef MESSAGE_PROTOCOL_HPP
#define MESSAGE_PROTOCOL_HPP

#include "General.h"
#include "UdpSocket.hpp"
#include "Eui.hpp"
#include "DebugMonitor.hpp"

namespace GatewayMessageProtocol
{
	enum MessageType {pushData = 0, pushAck = 1, pullData = 2, pullResponse = 3, pullAck = 4, numberOfMessageTypes};

	/*
	Respond to pushData with pushAck
	Respond to pullData with pullAck
	*/
	const uint versionOffset = 0;
	const uint tokenOffset = 1;
	const uint messageTypeOffset = 3;
	const uint16 headerLength = 4;
	const uint16 maxTransmittedUdpMessage = 1500;

	inline uint16 Read2ByteValue(uint8 const input[]) {return (uint16(input[0] << 8)) | uint16(input[1]);}
	uint32 Read4ByteValue(uint8 const input[]);
	uint64 Read8ByteValue(uint8 const input[]);

	//return value is pointer to location after output field
	uint8* Write4ByteValue(uint8 output[], uint32 input);
	uint8* Write8ByteValue(uint8 output[], uint64 input);

	class Parser;
	class SyntaxError //Exception
	{
	private:
		Parser&		parser;
		std::string	description;

	public:
		SyntaxError(Parser* const myParser, char const myDescription[] = "")
			: parser(*myParser), description(myDescription)
		{}

		SyntaxError(Parser* const myParser, std::string const& myDescription)
			: parser(*myParser), description(myDescription)
		{}

		SyntaxError(Parser* const myParser, std::stringstream const& myDescription)
			: parser(*myParser), description(myDescription.str())
		{}

		char const* Description() const {return description.c_str();}
	};

	class Parser
	{
	private:
		UDP::Socket& socket;
		uint8 const* const receivedData;
		uint8 const* const dataEnd;	//one past data
		uint64 const thisServerReceiveTime_ms;
		sockaddr_in const& source;	// warning - this is a reference

	public:
		///myReceivedData[myLength] must equal zero
		Parser(UDP::Socket& mySocket, uint8 const myReceivedData[], uint16 myLength, uint64 myThisServerReceiveTime_ms, sockaddr_in const& mySource)
			: socket(mySocket), receivedData(myReceivedData), dataEnd(myReceivedData + myLength), thisServerReceiveTime_ms(myThisServerReceiveTime_ms), source(mySource)
		{
			try
			{
				Parse();
			}
			catch (SyntaxError const& e)
			{
				if (Debug::Print(Debug::monitor))
				{
					std::stringstream errorText;

					errorText << "Message Protocol syntax error " << e.Description();

					Debug::Write(errorText);
				}
			}
		}

		uint8 Version() const {return receivedData[versionOffset];}
		uint16 Token() const {return uint16((receivedData[tokenOffset+1]) << 8) | receivedData[tokenOffset];}
		MessageType Type() const {return MessageType(receivedData[messageTypeOffset]);}
		uint8 const* DataPayload() const {return receivedData + headerLength + ((Type() != pullResponse) ? euiBytes : 0);}
		uint16 PacketLength() const {return uint16(dataEnd - receivedData);}
		sint16 DataPayloadLength() const {return sint16(PacketLength() - headerLength - euiBytes);}
		uint8 const* GatewayEui() const {return &receivedData[headerLength];}

	private:
		void Parse();
		void ReactToPushDataPacket() const;
		void ReactToPullDataPacket() const;
		void ReactToPushAckPacket() const {ReactToAcknowledgePacket();}
		void ReactToPullAckPacket() const {ReactToAcknowledgePacket();}
		void ReactToAcknowledgePacket() const;	//only called by ReactToPushAckPacket() and ReactToPullAckPacket()
		void ReactToPullResponsePacket() const;	//normally this is an empty function
		void TransmitAcknowledgement(UDP::Socket const& socket, MessageType msgType) const; // only if rx message is confirmable
	};
	
	void TransmitMessage(UDP::Socket const& socket, MessageType msgType, sockaddr_in const& destination, EuiType const& gatewayEui, uint16 token, uint8 const payload[], uint16 payloadLength); // Do not use for ACK messages
	//pullResponse token should always be zero
}

#endif

