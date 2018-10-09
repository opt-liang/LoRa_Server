/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef LORA_HPP
#define LORA_HPP

#include "General.h"
#include "Eui.hpp"
#include "ExceptionClass.hpp"
#include "DataRecord.hpp"
#include "ValueWithValidity.hpp"
#include "TimeRecord.hpp"
#include "Utilities.hpp"
#include <memory.h>
#include <limits.h>
#include <string>


//lint -e758 global enum not referenced
namespace LoRa
{
	const unsigned			authenticationKeyBytes = 16;
	const unsigned			authenticationBlockBytes = 16;
	const unsigned			cypherKeyBytes = 16;
	const unsigned			encryptionBlockBytes = 16;
	const unsigned			numberOfMoteWindows = 2;
	const uint32			moteWindowTime_us[numberOfMoteWindows] = {1000000UL, 2000000UL};
	const uint32 			moteJoinRequestWindowTime_us = 5 * 1000 * 1000;	//5 sec
	static const sint32		assumedSnrOfMissingFrames_cB = -250;
	const sint16			snrMargin_cB = 100;
	const uint32			maxDataRate = 100000000;

	const unsigned			defaultGatewayTransmitAntenna = 0;
	const unsigned			maxTransmitRetries = 5;
	const unsigned			minMoteRetransmissionDelay_ms = 3000;
	const uint16			defaultMoteChannelMask = 0x7;
	const uint16			defaultMoteChannelMaskControl = 0x6;
	const uint16			defaultMoteTransmissionsOfUnacknowledgedUplinkFrame = 1;	//0 or 1 mean transmit once

	const uint8				networkAddressBits = 25;
	const uint32			networkAddressMask = 0x1FFFFFFUL;	//25 bits
	const uint32			invalidNetworkAddress = ~uint32(0);

	inline uint32 CalculateNetworkId(uint32 networkAddress) {return networkAddress >> LoRa::networkAddressBits;}

	const uint32			gatewayKeepAlivePeriod_ms = 3 * 24 * 60 * 60 * 1000;
	const uint32			gatewayTimeoutMultiple = 6;
	const uint8				macCommandPort = 0;
	const uint32			defaultFskFrequencyDeviation = 25000;
	const uint32			defaultFskDataRate = 50000;

	//Frame length
	static const uint16 	micBytes = 4;	//message integrity code

	//Data frame length
	static const uint16 	macHeaderLength = 1;
	static const uint16 	minDataHeaderLength = 7;
	static const uint16 	headerOptionLengthMask = 0x0F;					//combined length of all options in a frame header
	static const uint16 	maxHeaderOptionLength = headerOptionLengthMask;
	static const uint16 	maxOptionRecordLength = 8;						//max length of an individual option - including identifier
	static const uint16 	portLength = 1;
	static const uint16 	maxFrameLength = 235;
	static const uint16 	maxDownstreamFrameLength = 64;
	static const uint16 	maxBase64TextLength = (4 * maxFrameLength)/3 +1;
	static const uint16 	maxDataBytes = maxFrameLength - (macHeaderLength + minDataHeaderLength + portLength + micBytes); //excluding port
	static const uint16 	maxDownstreamDataBytes = maxDownstreamFrameLength - (macHeaderLength + minDataHeaderLength + portLength + micBytes); //excluding port

	enum FrameType
	{
		joinRequestFrame = 0,
		joinAcceptFrame = 1,
		dataUnconfirmedFrameUp = 2,
		dataUnconfirmedFrameDown = 3,
		dataConfirmedFrameUp = 4,
		dataConfirmedFrameDown = 5,
		numberOfFrameTypes
	};

	enum FrameCopyType
	{
		first,				//first time this sequence number has been received
		duplicate,			//this frame is a duplicate of the previously received frame - received with a delay smaller than the gateway to node transmission delay
		invalidDelay,		//this frame is a duplicate of the previously received frame - received outside the acceptable duplicate frame delay but too early to be a retransmission
		retransmission,		//this frame is a retransmission of the previously received frame - received with a delay greater than the gateway to node transmission delay
		resetDetected,		//sequence number has gone backwards
		failedAuthentication
	};

	inline char const* FrameCopyTypeText(FrameCopyType type)
	{
		switch(type)
		{
		case first:					return "First";
		case duplicate:				return "Duplicate";
		case invalidDelay:			return "InvalidDelay";
		case retransmission:		return "Retransmission";
		case resetDetected:			return "ResetDetected";
		case failedAuthentication:	return "FailedAuthentication";
		default:					return "Invalid frame copy type";
		}
	}

	enum OptionId {linkCheck = 2, linkAdr = 3, devStatus = 6};
	enum ModulationType {loRaMod, fskMod};
	typedef ValueWithValidity<ModulationType>	ValidModulationType;

	static const uint maxNumberValidAdrDataRatesInAnyRegion = 6;
	typedef enum
	{
		//lint --e{849}  Symbol 'LoRa::SpreadingFactor::defaultSpreadingFactor' has same enumerator value '12' as enumerator 'maxSpreadingFactor')
		minSpreadingFactor = 7, 
		maxSpreadingFactorInAnyRegion = 12
	} SpreadingFactor;
	typedef ValueWithValidity<SpreadingFactor>	ValidSpreadingFactor;

	enum Region {americas902 = 0, china779 = 1, europe433 = 2, europe863 = 4, numberOfRegions, unknownRegion = numberOfRegions};	//do not change the values - they are stored in the database
	typedef ValueWithValidity<Region> ValidRegion;
	extern const ValidRegion invalidRegion;

	class CypherKey : public DataRecordFL<uint16, LoRa::cypherKeyBytes>
	{
	public:
		CypherKey() {Invalidate();}
		CypherKey(uint8 const myKey[]) : DataRecordFL<uint16, LoRa::cypherKeyBytes>(myKey)
		{}

		CypherKey(char const text[])
		{
			if (!ConvertFixedLengthHexTextToBinaryArray(text, DataNonConst(), Length(), true))
				Invalidate();
		}
		void Invalidate() {Initialise(0xFF);}
		bool Valid() const
		{
			for (uint i = 0; i < LoRa::cypherKeyBytes; i++)
			{	//test for any non 0xff byte
				if (*(Data() + i) != 0xFF)
					return true;
			}
			return false;
		}

		CypherKey& operator=(uint8 const myKey[]) {SetData(myKey); return *this;}	//Array of bytes
		CypherKey& operator=(char const myKeyText[])	//character string
		{
			if (!ConvertFixedLengthHexTextToBinaryArray(myKeyText, DataNonConst(), Length(), true))
				Invalidate();

			return *this;
		}

		std::string CastToString(char separator = ':') const
		{
			std::string result = ConvertBinaryArrayToHexText(Data(), Length(), true, separator);
			return result;
		}

		operator std::string() const {return CastToString();}
	};

	//Functions to write to & read from little endian frame - from endian agnostic machine
	//return is read value
	inline uint16 Read2ByteValue(uint8 const input[]) {return uint16(input[0]) | (uint16(input[1]) << 8);}
	uint32 Read3ByteValue(uint8 const input[]);
	uint32 Read4ByteValue(uint8 const input[]);
	uint64 Read8ByteValue(uint8 const input[]);

	//return value is pointer to location after output field
	uint8* Write1ByteValue(uint8 output[], uint8 input);
	uint8* Write2ByteValue(uint8 output[], uint16 input);
	uint8* Write3ByteValue(uint8 output[], uint32 input);
	uint8* Write4ByteValue(uint8 output[], uint32 input);
	uint8* Write8ByteValue(uint8 output[], uint64 input);

	void EncryptPayload(CypherKey const& key, uint8 const in[], uint16 inputDataLength, uint32 address, bool up, uint32 sequenceNumber, uint8 out[]);	//data is encrypted.  Out may be set to equal In
	inline void DecryptPayload(CypherKey const& key, uint8 const in[], uint16 inputDataLength, uint32 address, bool up, uint32 sequenceNumber, uint8 out[]) {EncryptPayload(key, in, inputDataLength, address, up, sequenceNumber, out);}
		//data is decrypted (same operation as encryption).  

	//output overwrites input
	inline void EncryptPayload(CypherKey const& key, uint8 data[], uint16 inputDataLength, uint32 address, bool up, uint32 sequenceNumber) 
		{EncryptPayload(key, data, inputDataLength, address, up, sequenceNumber, data);}
	inline void DecryptPayload(CypherKey const& key, uint8 data[], uint16 inputDataLength, uint32 address, bool up, uint32 sequenceNumber)	//data is decrypted (same operation as encryption).  
		{DecryptPayload(key, data, inputDataLength, address, up, sequenceNumber, data);}

	//LoRa specification requires that Network server encrypts and decrypts using the aes128 decryption algorithm so CryptJoinServer is used by the server and CryptJoinMote is used by the Mote
	void CryptJoinServer(CypherKey const& key, uint8 const input[], uint16 length, uint8 output[]); 
	// note LoRa::cypherKeyBytes is longer than join request/complete message.  LoRa spec requires that the server uses decrypt to encrypt as well as decrypt

	void CryptJoinMote(CypherKey const& key, uint8 const input[], uint16 length, uint8 output[]);

	CypherKey GenerateSessionKey(bool generateNetworkKey, CypherKey const& applicationKey, uint32 networkId, uint32 applicationNonce, uint16 deviceNonce);

	class DataRate
	{
	public:
		ModulationType				modulation;
		ValidValueUint32			bandwidth_Hz;	// bits per second if modulation == fskMod, LoRa modulation bandwdith if modulation == loraMod
		ValidSpreadingFactor		spreadingFactor;
		static const uint32			highBandwidthThreshold_Hz = 500 * 1000;

		DataRate() : modulation(loRaMod) {}
		DataRate(ModulationType myModulation, uint32 myBandwidth_Hz, uint16 mySpreadingFactor)
			:modulation(myModulation), bandwidth_Hz(myBandwidth_Hz), spreadingFactor(SpreadingFactor(mySpreadingFactor))
		{}
		DataRate(char const text[]) {*this = text;}
		DataRate(std::string const& text) {*this = text.c_str();}
		operator std::string() const;
		DataRate& operator=(DataRate const& other) {modulation = other.modulation; bandwidth_Hz = other.bandwidth_Hz; spreadingFactor = other.spreadingFactor; return *this;}
		DataRate& operator=(char const text[]);	//text need not be null terminated "SFn[n]BWn[nn][K|M" e.g. SF12BW500K or SF9BW500
		LoRa::ModulationType Modulation() const {return modulation;}

		bool Valid() const {return bandwidth_Hz.Valid() && (modulation == fskMod || spreadingFactor.Valid());}
		void SetInvalid() {bandwidth_Hz.SetInvalid(); spreadingFactor.SetInvalid();}

		bool IsHighBandwidth() const {return bandwidth_Hz.Valid() && (bandwidth_Hz.Value() >= highBandwidthThreshold_Hz);}
	};

	uint8 TranslateDataRateToDRFrameHeaderNibble(LoRa::Region region, LoRa::DataRate const& dataRate, bool downstream, bool highBandwidth);
	//Translates a datarate to a (region appropriate) nibble for use in a LoRa frame
	//highBandwidth is only used when region == americas902
	LoRa::DataRate TranslateFrameHeaderNibbleToDataRate(LoRa::Region region, uint8 dataRateNibble);

	class CodeRate
	{
	public:
		ValidValueUint16 carried;	//useful bits
		ValidValueUint16 total;		//useful bits plus error detection or correction bits

		CodeRate() {}
		CodeRate(uint16 myCarriedBits, uint16 myTotalBits)
			: carried(myCarriedBits), total(myTotalBits)
		{}
		CodeRate(char const text[]) {*this = text;}
		CodeRate(std::string const& text) {*this = text.c_str();}

		bool Valid() const {return carried.Valid() && total.Valid();}
		void SetInvalid() {carried.SetInvalid(); total.SetInvalid();}
		operator std::string() const;
		CodeRate& operator=(CodeRate const& other)
		{
			if (other.Valid())
			{
				carried = other.carried;
				total = other.total;
			}
			else
			{
				carried.SetInvalid();
				total.SetInvalid();
			}
			return *this;
		}
		CodeRate& operator=(char const text[]);	//text need not be null terminated "SFn[n]BWn[nn][K|M" e.g. SF12BW500K or SF9BW500
	};

	template <typename TSize, TSize maxLength> class FrameRecord : public DataRecordVL<TSize, maxLength>
	{
	public:
		FrameRecord() {}

		FrameRecord(uint8 const myData[], uint16 myLength)
			: DataRecordVL<TSize, maxLength>(myData, myLength)
		{}

		FrameRecord(FrameRecord const& other) 
			: DataRecordVL<TSize, maxLength>(other)
		{}

		void Append2ByteValue(uint16 input) {Write2ByteValue(DataRecordVL<TSize, maxLength>::FirstUnusedByte(), input);DataRecordVL<TSize, maxLength>::IncreaseLength(2);}
		void Append3ByteValue(uint32 input) {Write3ByteValue(DataRecordVL<TSize, maxLength>::FirstUnusedByte(), input);DataRecordVL<TSize, maxLength>::IncreaseLength(3);}
		void Append4ByteValue(uint32 input) {Write4ByteValue(DataRecordVL<TSize, maxLength>::FirstUnusedByte(), input);DataRecordVL<TSize, maxLength>::IncreaseLength(4);}
		void Append8ByteValue(uint64 input) {Write8ByteValue(DataRecordVL<TSize, maxLength>::FirstUnusedByte(), input);DataRecordVL<TSize, maxLength>::IncreaseLength(8);}
	};

	class FrameDataRecord : public FrameRecord<uint16, LoRa::maxFrameLength>
	{
	public:
		FrameDataRecord() {}

		FrameDataRecord(uint8 const myData[], uint16 myLength)
			: FrameRecord<uint16, LoRa::maxFrameLength>(myData, myLength)
		{}

		FrameDataRecord(FrameRecord<uint16, LoRa::maxFrameLength> const& other) 
			: FrameRecord<uint16, LoRa::maxFrameLength>(other)
		{}
		FrameDataRecord& operator=(FrameDataRecord const& other) {SetData(other.Data(), other.Length()); return *this;}
	};

	typedef DataRecordVL<uint16, LoRa::maxOptionRecordLength> OptionRecord;

	OptionRecord GenerateAdrCommand(uint8 power, uint8 dataRate, uint16 channelMask, uint8 channelMaskControl, uint8 transmissionsOfUnacknowledgedUplinkFrame);

	class Frame : public FrameDataRecord
	{
	public:
		static const uint8  moteAddressBytes = 4;

		static const uint16 macHeaderOffset = 0;
		static const uint16 addressOffset = macHeaderOffset + 1;
		static const uint16 controlOffset = addressOffset + moteAddressBytes;
		static const uint16 counterOffset = controlOffset + 1;
		static const uint16 optionOffset = counterOffset + 2;

		//Message header format
		static const uint8 typeMask = 0xe0;
		static const uint8 typeShift = 5;
		static const uint8 versionMask = 0x3; // aka Major version

		//Control byte format
		static const uint8 adrAckReqMask = 0x40;
		static const uint8 ackMask = 0x20;
		static const uint8 adrEnabledMask = 0x80;

		Frame() {}
		virtual ~Frame() {}

		FrameType Type() const {return FrameType((Data()[Frame::macHeaderOffset] & typeMask) >> typeShift);}
		std::string const TypeText() const;
		uint8 Version() const {return uint8(Data()[Frame::macHeaderOffset] & versionMask);}
		uint32 Address() const {return Read4ByteValue(&Data()[addressOffset]);}
		uint16 SequenceNumber() const {return Read2ByteValue(&Data()[counterOffset]);}
		bool AdrEnabled() const {return (Data()[controlOffset] & adrEnabledMask) ? true : false;}
		bool AdrAckRequest() const {return (Data()[controlOffset] & adrAckReqMask) ? true : false;}
		bool RequestingResponse() const {return (IsDataConfirmedFrame() || AdrAckRequest()) ? true : false;}
		bool Ack() const {return (Data()[controlOffset] & 0x20) ? true : false;}
		static uint16 MinHeaderLength() {return macHeaderLength + minDataHeaderLength;}
		uint8 const* OptionStart() const {return Data() + MinHeaderLength();}
		uint16 OptionLength() const {return (Data()[LoRa::Frame::controlOffset]) & headerOptionLengthMask;}
		uint16 HeaderLength() const {return MinHeaderLength() + OptionLength();}
		uint8 Port() const {return Data()[HeaderLength()];}

		bool IsDataConfirmedFrame() const {FrameType frameType = Type(); return (frameType == dataConfirmedFrameUp) || (frameType == dataConfirmedFrameDown);}
		bool IsDataUnconfirmedFrame() const {FrameType frameType = Type(); return (frameType == dataUnconfirmedFrameUp) || (frameType == dataUnconfirmedFrameDown);}
		bool IsUpFrame() const {FrameType frameType = Type(); return (frameType == dataConfirmedFrameUp) || (frameType == dataUnconfirmedFrameUp);}
		static bool IsDataFrameType(FrameType frameType) {return (frameType > joinAcceptFrame) && (frameType < numberOfFrameTypes);}
		bool IsDataFrame() const {return IsDataFrameType(Type());}
		bool IsJoinRequestFrame() const{return Type() == LoRa::joinRequestFrame;}
		bool IsJoinAcceptFrame() const{return Type() == LoRa::joinAcceptFrame;}
		bool IsJoinFrame() const {return (Type() | 1) == LoRa::joinAcceptFrame;}	// LoRa::joinRequestFrame OR LoRa::joinAcceptFrame

		uint16 PortAndPayloadLength() const 
		{
			sint16 result = sint16(Length() - HeaderLength() - micBytes);

			return (result > 0) ? uint16(result) : 0;
		}

		uint16 PayloadLength() const
		{
			uint16 portAndPayloadLength = PortAndPayloadLength();

			return (portAndPayloadLength > portLength) ? portAndPayloadLength - portLength : 0;
		}

		uint16 AuthenticatedLength() const {return (Length() >= micBytes) ? (Length() - micBytes) : 0;}

		uint8 const* Payload() const {return &Data()[HeaderLength() + portLength];}
		uint8* Payload() {return DataNonConst() + HeaderLength() + portLength;}
		uint8 const* Mic() const {return Data() + Length() - micBytes;}

		//Join request packet length
		static const uint16 devNonceBytes = 2;
		static const uint16 joinRequestBytes = macHeaderLength + 2 * euiBytes + devNonceBytes + micBytes;

		//Join accept packet length
		static const uint16 appNonceBytes = 3;
		static const uint16 netIdBytes = 3;
		static const uint16 channelFrequencyListBytes = 16;
		static const uint16 minJoinRequestBytes = macHeaderLength + 2 * sizeof(EuiType) + devNonceBytes + micBytes;
		static const uint16 minJoinAcceptBytes = macHeaderLength + appNonceBytes + netIdBytes + moteAddressBytes + micBytes;
		static const uint16 joinAcceptBytes = minJoinAcceptBytes + 2 + channelFrequencyListBytes;
	};

	void GenerateDataFrameIntegrityCode(CypherKey const& key, uint8 const input[], uint16 dataLength, uint32 address, bool up, uint32 sequenceNumber, uint8 output[micBytes]);
	void GenerateJoinFrameIntegrityCode(CypherKey const& key, uint8 const input[], uint16 dataLength, uint8 output[micBytes]);

	class FrameApplicationData : public FrameDataRecord
	{
	private:
		bool					acknowledgeRequested;
		ValidValueUint8			port;

	public:
		FrameApplicationData(bool myAcknowledgeRequested = true) // myPort == LoRa::macCommandPort is an illegal value
			:acknowledgeRequested(myAcknowledgeRequested)
		{}

		FrameApplicationData(uint8 const myData[], uint16 myLength, uint8 myPort, bool myAcknowledgeRequested = true) // myPort == LoRa::macCommandPort is an illegal value
			: FrameDataRecord(myData, myLength), port(myPort),
			acknowledgeRequested(myAcknowledgeRequested)
		{}

		bool AcknowledgeRequested() const {return acknowledgeRequested;}
		void AcknowledgeRequested(bool a) {acknowledgeRequested = a;}

		uint8 Port() const {return port;}
		void Port(uint8 newPort) {port = newPort;}
		bool IsUserData() const {return Valid() && Port() != LoRa::macCommandPort;}
		bool IsMacCommand() const {return Valid() && Port() == LoRa::macCommandPort;}
		bool Valid() const {return port.Valid();}
		bool Empty() const {return Length() == 0;}
		void Set(uint8 const dataIn[], uint16 lengthIn, uint8 portIn)	{SetData(dataIn, lengthIn); Port(portIn);}
		void Set(FrameApplicationData const& other) {Set(other.Data(), other.Length(), other.Port());}
		FrameApplicationData& operator=(FrameApplicationData const& other) {Set(other); return *this;}
	};

	class FrameApplicationDataSQ : public FrameApplicationData	//include a sequence number
	{
	private:
		ValidValueUint32		sequenceNumber;

	public:
		FrameApplicationDataSQ(bool myAcknowledgeRequested = true) // LoRa::macCommandPort is an illegal value
			:FrameApplicationData(myAcknowledgeRequested)
		{}

		FrameApplicationDataSQ(uint8 const myData[], uint16 myLength, uint8 myPort, bool myAcknowledgeRequested = true)
			: FrameApplicationData(myData, myLength, myPort, myAcknowledgeRequested)
		{}

		FrameApplicationDataSQ(uint8 const myData[], uint16 myLength, uint8 myPort, uint32 mySequenceNumber, bool myAcknowledgeRequested = true)
			: FrameApplicationData(myData, myLength, myPort, myAcknowledgeRequested), sequenceNumber(mySequenceNumber)
		{}

		ValidValueUint32 const& SequenceNumber() const {return sequenceNumber;}
		void SequenceNumber(ValidValueUint32 const& s) {sequenceNumber = s;}
		bool PortValid() const {return FrameApplicationData::Valid();}
		bool Valid() const {return sequenceNumber.Valid() && PortValid();}
		void Clear() {FrameDataRecord::Clear(); sequenceNumber.SetInvalid();}

		FrameApplicationDataSQ& operator=(FrameApplicationDataSQ const& other) {Set(other.Data(), other.Length(), other.Port()), sequenceNumber = other.SequenceNumber(); return *this;}
	};
}

int operator==(LoRa::DataRate const& l, LoRa::DataRate const& r);
inline int operator!=(LoRa::DataRate const& l, LoRa::DataRate const& r) {return !(l == r);}
int operator>(LoRa::DataRate const& l, LoRa::DataRate const& r);

#endif

