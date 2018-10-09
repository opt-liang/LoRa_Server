/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef GATEWAY_NC_HPP
#define GATEWAY_NC_HPP

#include "General.h"
#include "BinarySearchVector.hpp"
#include "ValueWithValidity.hpp"
#include "LoRa.hpp"
#include "Position.hpp"
#include "Utilities.hpp"
#include "Ip.h"

class Gateway: public BinarySearchVector::ElementTemplate<Gateway, EuiType>
{
public:
	static const uint16 maxNumberOfGPSReadingsUsedToFindAverage;
	static const uint16 minNumberOfGPSReadingsUsedToFindAverage;
	static const uint32 deletionTimeout_ms;
	static const uint32 elementTickPeriod_ms;
	static const sint16 defaultTransmitPower_dBm;
	static const sint16 maxTransmitPower_dBm;
	static const uint16 defaultDelayToNetworkServer_ms;
	static const uint16 defaultDelayFromNetworkServer_ms;
	static const uint64 nvStorageUpdatePeriod_ms = 60 * 5 * 1000;	//time between store operations in ms

	struct PositionStatus
	{
		Position					position;
		ValidValueBool				gps;
		ValidValueUint64			lastUpdateTime_ms;
	};

private:
	PositionStatus				positionStatus;
	LoRa::ValidRegion			region;

public:
	Gateway(EuiType eui)
		:
		BinarySearchVector::ElementTemplate<Gateway, EuiType>(eui)
	{
	}

	void StatusReceived(ValidValueBool const& gps, Position const& position, LoRa::ValidRegion const& region);
	PositionStatus GetPositionStatus() const {return positionStatus;}
	LoRa::ValidRegion const& GetRegion() const {return region;}
};

class GatewayList : public BinarySearchVector::List<Gateway, EuiType>
{
public:
	GatewayList(bool initialise, uint32 myListTickPeriod_ms);
	Gateway* GetById(EuiType eui) const {return static_cast<Gateway*>(BinarySearchVector::List<Gateway, EuiType>::GetById(eui));} 	//Gateway is locked
	Gateway* GetByIndex(uint32 index) const {return static_cast<Gateway*>(BinarySearchVector::List<Gateway, EuiType>::GetByIndex(index));} 	//Gateway is locked

	void StatusReceived(EuiType eui, ValidValueBool const& gps, Position const& position, LoRa::ValidRegion const& region)
	{
		if ((eui == invalidEui) || (eui == nullEui))
			return;	//invalid (null) gateway - should never happen but avoids updating a non-existent record

		//lint --e{1774}  (Info -- Could use dynamic_cast to downcast polymorphic type
		Gateway* gateway = GetById(eui);	// gateway is locked

		if (!gateway)
		{
			//newly visible gateway
			gateway = new Gateway(eui);

			Add(gateway);
			gateway->Lock();
		}

		gateway->StatusReceived(gps, position, region);
		gateway->Unlock();
	}

	Gateway::PositionStatus GetPositionStatus(EuiType eui) const;
	LoRa::ValidRegion GetRegion(EuiType eui);
};

//lint -e{1762} Info -- Member function 'BinarySearchVectorDebug::ElementTemplate<Gateway, EuiType>::TemplatedElementTick(void)' could be made const)
namespace BinarySearchVector
{
	template<> inline void ElementTemplate<Gateway, EuiType>::TemplatedElementTick(void)
	{
	}
}


#endif

