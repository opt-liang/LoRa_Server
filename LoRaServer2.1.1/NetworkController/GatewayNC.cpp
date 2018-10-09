/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "GatewayNC.hpp"
#include "Utilities.hpp"

const uint16 Gateway::maxNumberOfGPSReadingsUsedToFindAverage = 20;
const uint16 Gateway::minNumberOfGPSReadingsUsedToFindAverage = 3;
const uint32 Gateway::deletionTimeout_ms = 3 * 60 * 60 * 1000;
const uint32 Gateway::elementTickPeriod_ms = 60 * 1000;
const sint16 Gateway::defaultTransmitPower_dBm = 27;
const sint16 Gateway::maxTransmitPower_dBm = 30;
const uint16 Gateway::defaultDelayToNetworkServer_ms = 350;
const uint16 Gateway::defaultDelayFromNetworkServer_ms = 350;

GatewayList::GatewayList(bool initialise, uint32 myListTickPeriod_ms)
	: BinarySearchVector::List<Gateway, EuiType>(myListTickPeriod_ms, Gateway::elementTickPeriod_ms, Gateway::deletionTimeout_ms, true)
	//lint --e{1566}  Warning -- member 'GatewayList::nonVolatile' (line 122, file C:\Build\LoRa\Gateway-GPS\Server\Gateway.hpp) might have been initialized by a separate function 
{}

void Gateway::StatusReceived(ValidValueBool const& gps, Position const& position, LoRa::ValidRegion const& myRegion)
{
	if (position.Valid())
	{
		positionStatus.position = position;
		positionStatus.lastUpdateTime_ms = GetMsSinceStart();
	}

	if (gps.Valid())
		positionStatus.gps = gps.Value();

	if (myRegion.Valid())
		region = myRegion;
}

Gateway::PositionStatus GatewayList::GetPositionStatus(EuiType eui) const
{
	Gateway* gateway = GetById(eui);

	Gateway::PositionStatus result;
	if (!gateway)
		return result;

	result = gateway->GetPositionStatus();

	gateway->Unlock();
	return result;
}

LoRa::ValidRegion GatewayList::GetRegion(EuiType eui)
{
	Gateway* gateway = GetById(eui);

	if (!gateway)
		return LoRa::invalidRegion;

	LoRa::ValidRegion result = gateway->GetRegion();

	gateway->Unlock();
	return result;
}

