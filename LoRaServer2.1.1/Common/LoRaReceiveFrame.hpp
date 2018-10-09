/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef LORA_RECEIVE_FRAME_HPP
#define LORA_RECEIVE_FRAME_HPP

#include "General.h"
#include "Eui.hpp"
#include "LoRa.hpp"
#include "MessageAddress.hpp"
#include "ValueWithValidity.hpp"


namespace LoRa
{
	class ReceivedFrame : public Frame
	{
	private:
		MessageAddress				gatewayAddress;
		EuiType						gatewayEui;
		Region						gatewayRegion;

		uint64 const				thisServerReceiveTime_ms;

	public:
		ValidValueSint16			signalStrength_cBm;
		ValidValueUint16			channel;	//only 8 bits needed but use 16 to avoid the value being printed as a character
		ValidValueUint32			gatewayReceiveTimestamp_us;	//timestampt of receipt (us) - unrelated to date and time
		TimeRecord					gatewayReceiveTime;
		ValidValueSint16			signalToNoiseRatio_cB; //SNR in cB (centiBells (10 * dB) only valid when modulation is LoRa::loRaMod
		DataRate					dataRate;
		CodeRate					codeRate;
		ValidValueUint32			frequency_Hz;	//Hz
		ValidValueUint16			rfChain;	//only 8 bits needed but use 16 to avoid the value being printed as a character
		ValidValueUint32			inferredSequenceNumber;

		ReceivedFrame(uint64 myThisServerReceiveTime_ms)
			: gatewayEui(0), gatewayRegion(unknownRegion),
			thisServerReceiveTime_ms(myThisServerReceiveTime_ms)
		{}

		void Receive();

		void GatewayAddress(MessageAddress const& a) {gatewayAddress = a;}
		MessageAddress const& GatewayAddress() const {return gatewayAddress;}
		void GatewayEui(EuiType const& other) {gatewayEui = other;}
		EuiType const& GatewayEui() const {return gatewayEui;}
		void GatewayRegion(Region r) {gatewayRegion = r;}
		Region GatewayRegion() const {return gatewayRegion;}

		uint64 ThisServerReceiveTime_ms() const {return thisServerReceiveTime_ms;}	// in ms value of GetMsSinceStart()

		bool Valid() const
			{return gatewayReceiveTimestamp_us.Valid() && dataRate.Valid() && 
				(dataRate.modulation == LoRa::fskMod || (codeRate.Valid()) && frequency_Hz.Valid() && rfChain.Valid());}

		uint32 Frequency_Hz() const {return frequency_Hz;}

		class MessageRejectException
		{
		private:
			static const uint16		noErrorNumber = SHRT_MAX;
			std::string				explanation;
			uint16					number;
			LoRa::Frame				frame;

		public:
			MessageRejectException(LoRa::Frame const& myFrame, char const myExplanation[] = "", uint16 myNumber = noErrorNumber)
				: explanation(myExplanation), number(myNumber), frame(myFrame)
			{
			}

			MessageRejectException(LoRa::Frame const& myFrame, std::string const& myExplanation = "", uint16 myNumber = noErrorNumber)
				: explanation(myExplanation), number(myNumber), frame(myFrame)
			{
			}

			~MessageRejectException()
			{
			}

			std::string const& Explanation() const {return explanation;}
			bool NumberSet() const {return number != noErrorNumber;}
			uint64 Number() const {return number;}

			LoRa::Frame const& Frame() const {return frame;}
		};
	};
}


#endif

