/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "JsonString.hpp"
#include "TransmissionRecord.hpp"
#include "Eui.hpp"
#include "Position.hpp"

void JSON::String::AddDataRate(LoRa::DataRate const& dataRate, bool addCommaBefore)
{
	std::stringstream text;

	if (dataRate.Modulation() == LoRa::loRaMod)
	{
		text << "SF" << dataRate.spreadingFactor << "BW" << (dataRate.bandwidth_Hz / 1000);

		AddTextValue("datr",text, addCommaBefore);
	}
	else
		AddUnsignedValue("datr", dataRate.bandwidth_Hz, addCommaBefore);
}


void JSON::String::AddPositionObject(bool positionIsFromGPS, Position const& position, bool addCommaBefore)
{
	OpenStructuredObject("posn", addCommaBefore);
	AddBooleanValue("gps", positionIsFromGPS, false);
	AddPosition(position);

	Close();
}


void JSON::String::AddPosition(Position const& position, bool addCommaBefore)
{
	if (position.latitude.Valid())
	{
		AddDoubleValue("lati", position.latitude, 5, addCommaBefore);
		addCommaBefore = true;
	}

	if (position.longitude.Valid())
	{
		AddDoubleValue("long", position.longitude, 5, addCommaBefore);
		addCommaBefore = true;
	}

	if (position.altitude.Valid())
	{
		AddDoubleValue("alti", position.altitude, 1, addCommaBefore);
		addCommaBefore = true;
	}

	if (position.toleranceHorizonal.Valid())
	{
		AddDoubleValue("tolh", position.toleranceHorizonal, 1, addCommaBefore);
		addCommaBefore = true;
	}

	if (position.toleranceVertical.Valid())
	{
		AddDoubleValue("tolv", position.toleranceVertical, 1, addCommaBefore);
		addCommaBefore = true;
	}
}


void JSON::String::AddAppObject(bool up, EuiType eui, ValidValueUint32 const& sequenceNumber, ValidValueUint16 const& token, LoRa::FrameApplicationData const& payload, MoteTransmitRecord const& transmitRecord, GatewayReceiveList const& gatewayList, bool addCommaBefore)
{
	OpenStructuredObject("app", addCommaBefore);
	AddEui("moteeui", eui, false);

	AddDirection(up);

	if (sequenceNumber.Valid())
		AddUnsignedValue("seqno", sequenceNumber);

	if (token.Valid())
		AddUnsignedValue("token", token);

	if (payload.Length() > 0)
		AddUserData(payload);

	if (transmitRecord.Valid())
		AddMoteTxMetaData(transmitRecord);

	if (!gatewayList.IsEmpty())
		AddGatewayRxMetaData(gatewayList);

	Close();	//app
}


void JSON::String::AddUserData(LoRa::FrameApplicationData const& payload, bool addCommaBefore)
{
	OpenStructuredObject("userdata", addCommaBefore);

	AddUnsignedValue("port", payload.Port(), false);

	AddDataAsBase64("payload", payload.Data(), payload.Length());

	Close();	//userdata
}


void JSON::String::AddMacCommand(EuiType mote, ValidValueUint16 const& token, LoRa::OptionRecord const& command, bool addCommaBefore)
{
	OpenStructuredObject("maccmd", addCommaBefore);
	AddEui("moteeui", mote, false);

	if (token.Valid())
		AddUnsignedValue("token", token);

	AddDataAsBase64("command", command.Data(), command.Length());
	Close();
}


void JSON::String::AddMacCommand(EuiType mote, ValidValueUint16 const& token, LoRa::FrameDataRecord const& command, bool addCommaBefore)
{
	OpenStructuredObject("maccmd", addCommaBefore);
	AddEui("moteeui", mote, false);

	if (token.Valid())
		AddUnsignedValue("token", token);

	AddDataAsBase64("command", command.Data(), command.Length());
	Close();
}


void JSON::String::AddMoteTxMetaData(MoteTransmitRecord const& record, bool addCommaBefore)
{
	OpenStructuredObject("motetx", addCommaBefore);

	bool writeComma = false;

	if (record.frequency_Hz.Valid())
	{
		AddFixedPointNumber("freq", false, record.frequency_Hz, 1000000UL, writeComma);
		writeComma = true;
	}

	if (record.dataRate.Valid())
	{
		AddDataRate(record.dataRate, writeComma);
		writeComma = true;
	}

	if (record.codeRate.Valid())
	{
		AddTextValue("codr", record.codeRate, writeComma);
		writeComma = true;
	}

	if (record.adrEnabled.Valid())
	{
		AddBooleanValue("adr", record.adrEnabled, writeComma);
		writeComma = true;
	}

	Close();
}


void JSON::String::AddGatewayRxMetaData(bool includeName, GatewayReceiveRecord const& record, bool addCommaBefore)
{
	if (includeName)
		OpenStructuredObject("gwrx", addCommaBefore);
	else
	{
		if (addCommaBefore)
			Comma();

		Open();
	}
	AddEui("eui", record.Eui(), false);

	if (record.receiveTime.Valid())
	{
		AddTextValue("time", record.receiveTime.JsonString(true));
		AddBooleanValue("timefromgateway", record.receiveTime.Accurate());
	}

	if (record.channel.Valid())
		AddUnsignedValue("chan", record.channel);

	if (record.rfChain.Valid())
		AddUnsignedValue("rfch", record.rfChain);

	if (record.signalStrength_cBm.Valid())
		AddSignedValue("rssi", record.signalStrength_cBm / 10);	//convert to dBm

	if (record.signalToNoiseRatio_cB.Valid())
		AddFixedPointNumber("lsnr", false, record.signalToNoiseRatio_cB, 10);

	Close();
}


void JSON::String::AddGatewayRxMetaData(GatewayReceiveList const& list, bool addCommaBefore)
{
	OpenArray("gwrx", addCommaBefore);

	list.Lock();

	GatewayReceiveList::List::const_iterator it = list.IteratorBegin();

	for (;it != list.IteratorEnd(); ++it)
		AddGatewayRxMetaData(false,**it, it != list.IteratorBegin());	// add a comma before every record except the first
	
	CloseArray();

	list.Unlock();
}

void JSON::String::AddMessageSent(EuiType mote, bool app, uint16 token, bool addCommaBefore)
{
	OpenStructuredObject("mote", addCommaBefore);
	AddEui("eui", mote, false);
	AddBooleanValue("app", app);
	AddUnsignedValue("msgsent", token);
	Close();
}


void JSON::String::AddResetDetected(EuiType mote, bool addCommaBefore)
{
	OpenStructuredObject("mote", addCommaBefore);
	AddEui("eui", mote, false);
	AddTextValue("resetdetected","");
	Close();
}


void JSON::String::AddAcknowledgementReceived(EuiType mote, bool app, bool addCommaBefore)
{
	OpenStructuredObject("mote", addCommaBefore);
	AddEui("eui", mote, false);
	AddBooleanValue("app", app);
	AddTextValue("ackrx","");
	Close();
}


void JSON::String::AddQueueLengthQuery(EuiType mote, bool app, bool addCommaBefore)
{
	OpenStructuredObject("mote", addCommaBefore);
	AddEui("eui", mote, false);
	AddBooleanValue("app", app);
	AddTextValue("qlenquery","");
	Close();
}


void JSON::String::AddQueueLength(EuiType mote, bool app, uint16 length, bool addCommaBefore)
{
	OpenStructuredObject("mote", addCommaBefore);
	AddEui("eui", mote, false);
	AddBooleanValue("app", app);
	AddUnsignedValue("qlen",length);
	Close();
}


void JSON::String::AddSequenceNumberRequest(EuiType mote, bool addCommaBefore)
{
	OpenStructuredObject("mote", addCommaBefore);
	AddEui("eui", mote, false);
	AddTextValue("seqnoreq","");
	Close();
}


void JSON::String::AddSequenceNumberGrant(EuiType mote, uint32 sequenceNumber, bool addCommaBefore)
{
	OpenStructuredObject("mote", addCommaBefore);
	AddEui("eui", mote, false);
	AddUnsignedValue("seqnogrant",sequenceNumber);
	Close();
}


void JSON::String::AddJoinNotificationAtoN(EuiType mote, EuiType app, bool addCommaBefore)
{
	OpenStructuredObject("mote", addCommaBefore);
	AddEui("eui", mote, false);
	OpenStructuredObject("join");

	AddEui("appeui", app, false);
	Close();
	Close();
}


void JSON::String::AddJoinRequestNtoA(LoRa::Frame const& frame, bool addCommaBefore)
{
	OpenStructuredObject("join", addCommaBefore);
	OpenStructuredObject("request", false);

	AddDataAsBase64("frame", frame.Data(), frame.Length(), false);

	Close();
	Close();
}


void JSON::String::AddJoinDetailsNtoA(EuiType moteEui, EuiType appEui, uint32 moteNetworkAddress, uint16 deviceNonce, bool addCommaBefore)
{
	OpenStructuredObject("join", addCommaBefore);
	AddEui("appeui", appEui, false);
	AddEui("moteeui", moteEui);
	OpenStructuredObject("details");

	AddUnsignedHexValue("moteaddr", moteNetworkAddress, false);
	AddUnsignedValue("devicenonce", deviceNonce);

	Close();	//details
	Close();	//join
}


void JSON::String::AddJoinAccept(EuiType moteEui, bool accept)
{
	OpenStructuredObject("join", false);
	AddEui("moteeui", moteEui, false);

	AddBooleanValue("accept", accept);

	Close();	//join
}


void JSON::String::AddJoinComplete(EuiType moteEui, LoRa::CypherKey const& networkSessionKey, LoRa::Frame const& frame)
{
	OpenStructuredObject("join", false);

	AddEui("moteeui", moteEui, false);
	OpenStructuredObject("complete");
	AddDataAsBase64("frame", frame.Data(), frame.Length(), false);
	AddUnsignedHexValue("networkkey", networkSessionKey.Data(), networkSessionKey.Length());

	Close();	//complete
	Close();	//join
}

