/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "CommandParserNS.hpp"
#include "Eui.hpp"
#include "GlobalDataNS.hpp"
#include "LoRaRegion.hpp"


#include "ExceptionClass.hpp"
#include "DebugMonitor.hpp"

#include <iostream>
#include <iomanip>

namespace
{
	std::string const spacer = "   ";
}

void CommandParserNS::ParsePrivate(std::string const& command)
{
	if (command == "mote")
		ParseMoteCommand();
	else if (command == "gateway")
		ParseGatewayCommand();
	else throw SyntaxError(*this);
}


void CommandParserNS::ParseMoteCommand()
{
	std::string const& command = wordStore.GetNextWord();	//holds the command word

	if (command == "add")
		ParseMoteAddCommand();
	else if (command == "reset")
		ParseMoteResetCommand();
	else if (command == "delete")
		ParseMoteDeleteCommand();
	else if (command == "set")
		ParseMoteSetCommand();
	else if (command == "maccmd")
		ParseMoteMacCommand();
	else if (command == "list")
		ParseMoteListCommand();

	else throw SyntaxError(*this);
}


void CommandParserNS::ParseMoteAddCommand()
{
	EuiType moteEui = wordStore.GetNextEui();

	if (Global::moteList.Exists(moteEui))
		throw ParameterError(*this, "Mote already exists");

	ValidValueEuiType appEui;
	LoRa::CypherKey key;
	ValidValueUint32 networkAddress;

	bool legacy = false;
	bool first = true;	// legacy can only be detected on first loop

	for (;; first = false)
	{
		std::string const& parameter = wordStore.GetNextWord();

		if (parameter.empty())
			break;

		else if (parameter == "app")
			appEui = GetExistingApplicationEui();

		else if (parameter == "key")
			key = wordStore.GetNextCypherKey();

		else if (parameter == "netaddr")
			networkAddress = wordStore.GetNextNetworkAddress();

		//does not comply to current syntax - may be legacy
		else if (first)
		{
			legacy = true;
			break;
		}
		else
			throw SyntaxError(*this);
	}

	if (legacy)
	{
		if (moteEui >= LoRa::invalidNetworkAddress)
			throw SyntaxError(*this);	//when using legacy command moteEui must be less than 2^32

		wordStore.ReturnWord(2);	//two words have been read - moteEUI and one other
		ParseAddMoteLegacyCommand();
		return;
	}

	if (!appEui.Valid())
		throw SyntaxError(*this, "Application must be specified");

	if (!key.Valid())
		key = Global::defaultAuthenticationKey;

	if (!networkAddress.Valid())
	{
		if (appEui < LoRa::invalidNetworkAddress)
			networkAddress = static_cast<uint32>(appEui.Value());
		else
			throw SyntaxError(*this, "\"netaddr\" parameter missing and EUI too big to be used as an assumed network address");
	}

	Global::CreateProvisionedMote(moteEui, appEui, networkAddress, key);
}


void CommandParserNS::ParseAddMoteLegacyCommand()
{
	EuiType moteEui = wordStore.GetNextEui();

	if (moteEui > LoRa::invalidNetworkAddress)
		throw ParameterError(*this, "EUI of provisioned mote cannot be greater than 32 bits");

	LoRa::CypherKey authenticationKey = wordStore.GetNextCypherKey(false);

	if (!authenticationKey.Valid())
		authenticationKey = Global::defaultAuthenticationKey;

	Global::CreateProvisionedMote(moteEui, nullEui, uint32(moteEui), authenticationKey);
}


void CommandParserNS::ParseMoteDeleteCommand()
{
	EuiType eui = wordStore.GetNextEui();

	if (!Global::DeleteProvisionedMote(eui))
		throw ParameterError(*this, "Mote does not exist");
}


void CommandParserNS::ParseMoteSetCommand()
{
	//Read command and check syntax
	EuiType eui = wordStore.GetNextEui();

	ValidValueUint32 dataRate;
	uint16 channelMask = Global::defaultMoteChannelMask;
	uint8 channelMaskControl = uint8(Global::defaultMoteChannelMaskControl);
	uint8 power = 1; //14dBm in Europe and 28dBm in Americas
	uint8 transmissionsOfUnacknowledgedUplinkFrame = uint8(Global::transmissionsOfUnacknowledgedUplinkFrame);

	for (;;)
	{
		std::string const& name = wordStore.GetNextWord();

		if (name.empty())
			break;

		if (name == "datarate" || name == "dr")
			dataRate = static_cast<uint8>(wordStore.GetNextUnsigned());

		else if (name == "mask")
			channelMask = static_cast<uint16>(wordStore.GetNextUnsigned(true));

		else if (name == "maskcontrol")
			channelMaskControl = static_cast<uint8>(wordStore.GetNextUnsigned());

		else if (name == "power")
			power = static_cast<uint8>(wordStore.GetNextUnsigned());

		else if (name == "uplinktxtries")
			transmissionsOfUnacknowledgedUplinkFrame = static_cast<uint8>(wordStore.GetNextUnsigned());

		else
			throw SyntaxError(*this, "Unrecognised parameter");
	}

	if (!dataRate.Valid())
		throw SyntaxError(*this, "Unable to read datarate (dr)");

	if (dataRate & ~0xF)
		throw ParameterError(*this, "Invalid datarate read");

	if (channelMask == 0)
		throw ParameterError(*this, "Invalid \'mask\' read");

	if (channelMaskControl & ~0x7)
		throw ParameterError(*this, "Invalid \'maskcontrol\' read");

	if (power & ~0xF)
		throw ParameterError(*this, "Invalid \'power\' code read");

	Mote* mote = Global::moteList.GetById(eui);
	if (!mote)
		throw ParameterError(*this, "mote does not exist");

	//Form MAC command
	LoRa::OptionRecord adrCommand = LoRa::GenerateAdrCommand(power, dataRate, channelMask, channelMaskControl, transmissionsOfUnacknowledgedUplinkFrame);

	//Transmit MAC command
	mote->SendMacCommand(adrCommand, invalidValueUint16);

	mote->Unlock();
}


void CommandParserNS::ParseMoteMacCommand()
{
	EuiType eui = wordStore.GetNextEui();

	if (wordStore.GetNextWord() != "data")
		throw SyntaxError(*this, "Expected \'data\'");

	std::string dataText = wordStore.ReadHexText();

	uint8 frameData[LoRa::maxOptionRecordLength + 1];
	uint16 frameDataBytes = ConvertVariableLengthHexTextToBinaryArray(dataText.c_str(), frameData, LoRa::maxOptionRecordLength + 1, false);

	if (frameDataBytes == 0)
		throw SyntaxError(*this, "Unable to interpret MAC command text");

	if (frameDataBytes > LoRa::maxOptionRecordLength)
		throw SyntaxError(*this, "MAC command text too long");

	LoRa::OptionRecord optionRecord;
	
	optionRecord.SetData(frameData, frameDataBytes);

	Mote* mote = Global::moteList.GetById(eui);
	if (!mote)
		throw ParameterError(*this, "mote does not exist");

	mote->SendMacCommand(optionRecord, invalidValueUint16);

	mote->Unlock();
}


void CommandParserNS::ParseMoteResetCommand()
{
	EuiType eui = wordStore.GetNextEui();

	//lint --e{1774} Info -- Could use dynamic_cast to downcast polymorphic type
	Mote* mote = Global::moteList.GetById(eui);

	if (!mote)
		throw ParameterError(*this);

	mote->ResetDetected();
	mote->Unlock();
}


void CommandParserNS::ParseMoteListCommand() const
{
	for (unsigned index = 0;; index++)
	{
		Mote* mote = Global::moteList.GetByIndex(index);	//mote is always locked

		if (!mote)
			break;

		std::stringstream output;

		output << AddressText(mote->Id()) << spacer <<
			AddressText(mote->ApplicationEui()) << spacer <<
			AddressText(mote->NetworkAddress()) << spacer << 
			mote->NetworkSessionKey().CastToString();

		mote->Unlock();

		Write(output);
	}
}


void CommandParserNS::ParseGatewayCommand()
{
	std::string const& command = wordStore.GetNextWord();	//holds the command word

	if (command == "add")
		ParseGatewayAddCommand();
	else if (command == "set")
		ParseGatewaySetCommand();
	else if (command == "delete")
		ParseGatewayDeleteCommand();
	else if (command == "list")
		ParseGatewayListCommand();

	else throw SyntaxError(*this);
}


void CommandParserNS::ParseGatewayAddCommand()
{
	EuiType eui = wordStore.GetNextEui();

	if (Global::gatewayList.Exists(eui))
		throw ParameterError(*this, "Gateway already exists");

	LoRa::Region region = LoRa::unknownRegion;
	Position position;
	ValidValueBool allowGpsToSetPosition;

	for (;;)
	{
		std::string name = wordStore.GetNextWord();

		if (name.empty())
			break;

		if (name == "region")
			region = ReadRegion();

		else if (name == "lat" || name == "lati")
			position.latitude = wordStore.GetNextCoordinate('N','S');

		else if (name == "long")
			position.longitude = wordStore.GetNextCoordinate('E','W');

		else if (name == "alt" || name == "alti")
			position.altitude = wordStore.GetNextCoordinate('m','\0');

		else if (name == "allowgps")
			allowGpsToSetPosition = wordStore.GetNextBoolean();
	}

	//allowGpsToSetPosition defaults to false if position configured
	if (position.Valid() && !allowGpsToSetPosition.Valid())
		allowGpsToSetPosition = false;

	Global::gatewayList.Add(eui, region, position, allowGpsToSetPosition);
}


void CommandParserNS::ParseGatewaySetCommand()
{
	EuiType eui = wordStore.GetNextEui();

	LoRa::Region region = LoRa::unknownRegion;
	Position position;
	ValidValueBool allowGpsToSetPosition;

	for (;;)
	{
		std::string name = wordStore.GetNextWord();

		if (name.empty())
			break;

		if (name == "region")
			region = ReadRegion();

		else if (name == "lat" || name == "lati")
			position.latitude = wordStore.GetNextCoordinate('N','S');

		else if (name == "long")
			position.longitude = wordStore.GetNextCoordinate('E','W');

		else if (name == "alt" || name == "alti")
			position.altitude = wordStore.GetNextCoordinate('m','\0');

		else if (name == "allowgps")
			allowGpsToSetPosition = wordStore.GetNextBoolean();
	}

	//allowGpsToSetPosition defaults to false if position configured
	if (position.Valid() && !allowGpsToSetPosition.Valid())
		allowGpsToSetPosition = false;

	Gateway* gateway = Global::gatewayList.GetById(eui);

	if (!gateway)
		throw ParameterError(*this, "Gateway not found");

	if (region != LoRa::unknownRegion)
		gateway->SetRegion(region);

	if (allowGpsToSetPosition.Valid())
		gateway->AllowGpsToSetPosition(allowGpsToSetPosition.Value());

	if (position.Valid())
		gateway->SetConfiguredPosition(position);

	gateway->Unlock();
}


void CommandParserNS::ParseGatewayDeleteCommand()
{
	EuiType eui = wordStore.GetNextEui();

	if (!Global::gatewayList.Exists(eui))
		throw ParameterError(*this, "Gateway is unknown");

	Global::gatewayList.DeleteById(eui);
}


void CommandParserNS::ParseGatewayListCommand() const
{
	for (unsigned index = 0;; index++)
	{
		Gateway* gateway = Global::gatewayList.GetByIndex(index);		//gateway is always locked

		if (!gateway)
			break;

		std::stringstream out;
		out << AddressText(gateway->Id()) << spacer << 
			std::setw(LoRa::MaxRegionText()) << LoRa::RegionText(gateway->Region()) << spacer << 
			std::setw(Position::TextWidth()) << gateway->GetPosition().Text(true, false);

			if (IsValid(gateway->PullIpAddress()))
				out << spacer << std::setw(0) << AddressText(gateway->PullIpAddress());

		gateway->Unlock();

		Write(out);
	}
}


void CommandParser::ParseSetPrivateCommand(std::string const& command)
{
	if (CaseInsensitiveEqual(command, Global::gatewayTxPower_dBm.Name()))
	{
		sint16 input = wordStore.GetNextSigned();

		if (input > Gateway::maxTransmitPower_dBm)
			throw ParameterError(*this, "Power too great");

		Global::gatewayTxPower_dBm = input;
	}

	else if (CaseInsensitiveEqual(command, Global::defaultAuthenticationKey.Name()))
		Global::defaultAuthenticationKey = wordStore.GetNextCypherKey();

	else if (CaseInsensitiveEqual(command, Global::defaultGatewayRegion.Name()))
		Global::defaultGatewayRegion.Read(wordStore.GetNextWord());
	else if (CaseInsensitiveEqual(command, Global::dataRateWindow1Americas902.Name()))
		Global::dataRateWindow1Americas902.Read(wordStore.GetNextWord());
	else if (CaseInsensitiveEqual(command, Global::dataRateWindow1China779.Name()))
		Global::dataRateWindow1China779.Read(wordStore.GetNextWord());
	else if (CaseInsensitiveEqual(command, Global::dataRateWindow1Eu433.Name()))
		Global::dataRateWindow1Eu433.Read(wordStore.GetNextWord());
	else if (CaseInsensitiveEqual(command, Global::dataRateWindow1Eu863.Name()))
		Global::dataRateWindow1Eu863.Read(wordStore.GetNextWord());
	else if (CaseInsensitiveEqual(command, Global::defaultMoteChannelMask.Name()))
		Global::defaultMoteChannelMask = wordStore.GetNextUnsigned(true);	//hex
	else if (CaseInsensitiveEqual(command, Global::defaultMoteChannelMaskControl.Name()))
		Global::defaultMoteChannelMaskControl = wordStore.GetNextUnsigned();
	else if (CaseInsensitiveEqual(command, Global::gatewayToNetworkServerMaxDelay_ms.Name()))
		Global::gatewayToNetworkServerMaxDelay_ms = wordStore.GetNextUnsigned();
	else if (CaseInsensitiveEqual(command, Global::timeToAssumeMoteLost_s.Name()))
		Global::timeToAssumeMoteLost_s = wordStore.GetNextUnsigned();
	else if (CaseInsensitiveEqual(command, Global::timeToAssumeMoteReset_s.Name()))
		Global::timeToAssumeMoteReset_s = wordStore.GetNextUnsigned();
	else if (CaseInsensitiveEqual(command, Global::moteMissSeqNoSearchLimit.Name()))
		Global::moteMissSeqNoSearchLimit = wordStore.GetNextUnsigned();
	else if (CaseInsensitiveEqual(command, Global::moteMissSeqNoSearchRetries.Name()))
		Global::moteMissSeqNoSearchRetries = wordStore.GetNextUnsigned();
	else if (CaseInsensitiveEqual(command, Global::moteReceiveWindow.Name()))
		Global::moteReceiveWindow = wordStore.GetNextUnsigned();
	else if (CaseInsensitiveEqual(command, Global::moteResetSeqNoSearchLimit.Name()))
		Global::moteResetSeqNoSearchLimit = wordStore.GetNextUnsigned();
	else if (CaseInsensitiveEqual(command, Global::networkServerToGatewayMaxDelay_ms.Name()))
		Global::networkServerToGatewayMaxDelay_ms = wordStore.GetNextUnsigned();
	else if (CaseInsensitiveEqual(command, "netdelayboth_ms")) //netdelayboth_ms isn't a configured value but is a short cut to two others
	{
		uint32 delay_ms = wordStore.GetNextUnsigned();
		Global::gatewayToNetworkServerMaxDelay_ms = delay_ms;
		Global::networkServerToGatewayMaxDelay_ms = delay_ms;
	}
	else if (CaseInsensitiveEqual(command, Global::transmissionsOfUnacknowledgedUplinkFrame.Name()))
		Global::transmissionsOfUnacknowledgedUplinkFrame = wordStore.GetNextUnsigned();
	else
		throw SyntaxError(*this);
}


void CommandParser::ParseSetListCommand() const
{
	std::stringstream text;

	text << std::endl <<
	WriteConfigurationValue(Global::allowRemoteConfiguration) << std::endl <<
	WriteConfigurationValue(Global::autoCreateMotes) << std::endl <<
	WriteConfigurationValue(Global::dataRateWindow1Americas902) << std::endl <<
	WriteConfigurationValue(Global::dataRateWindow1China779) << std::endl <<
	WriteConfigurationValue(Global::dataRateWindow1Eu433) << std::endl <<
	WriteConfigurationValue(Global::dataRateWindow1Eu863) << std::endl <<
	WriteConfigurationValue(Global::defaultAuthenticationKey) << std::endl <<
	WriteConfigurationValue(Global::defaultGatewayRegion) << std::endl <<
	WriteConfigurationValue(Global::defaultMoteChannelMask) << std::endl <<
	WriteConfigurationValue(Global::defaultMoteChannelMaskControl) << std::endl <<
	WriteConfigurationValue(Global::gatewayTxPower_dBm) << std::endl <<
	WriteConfigurationValue(Global::moteReceiveWindow) << std::endl <<
	WriteConfigurationValue(Global::moteResetSeqNoSearchLimit) << std::endl <<
	WriteConfigurationValue(Global::moteMissSeqNoSearchLimit) << std::endl <<
	WriteConfigurationValue(Global::moteMissSeqNoSearchRetries) << std::endl <<
	WriteConfigurationValue(Global::networkServerToGatewayMaxDelay_ms) << std::endl <<
	WriteConfigurationValue(Global::gatewayToNetworkServerMaxDelay_ms) << std::endl <<
	WriteConfigurationValue(Global::timeToAssumeMoteLost_s) << std::endl <<
	WriteConfigurationValue(Global::timeToAssumeMoteReset_s) << std::endl <<
	WriteConfigurationValue(Global::transmissionsOfUnacknowledgedUplinkFrame) << std::endl;
	Write(text);
}


void CommandParser::Write(std::string const& text) const
{
	Global::commandLineInterface.Write(text);
}

void CommandParser::Broadcast(std::string const& text) const
{
	Global::commandLineInterface.Broadcast(text);
}

void CommandParser::EchoLine(char const /*line*/[]) const
{
}


LoRa::Region CommandParserNS::ReadRegion()
{
	std::string regionName = wordStore.GetNextWord();

	if (regionName.empty())
		throw SyntaxError(*this, "Region not given");

	LoRa::Region region = LoRa::ReadRegion(regionName);

	if (region >= LoRa::numberOfRegions)
		throw SyntaxError(*this, "Unable to read region");

	return region;
}


