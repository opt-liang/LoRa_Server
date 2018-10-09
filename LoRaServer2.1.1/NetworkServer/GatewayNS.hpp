/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef GATEWAY_NS_HPP
#define GATEWAY_NS_HPP

#include "General.h"
#include "BinarySearchVectorNV.hpp"
#include "GatewayStatus.hpp"
#include "ValueWithValidity.hpp"
#include "ConsolidatedPosition.hpp"
#include "LoRa.hpp"
#include "LoRaReceiveFrame.hpp"
#include "Utilities.hpp"
#include "Ip.h"
#include "WaitingFrame.hpp"
#include "JsonString.hpp"
#include "TransmissionRecord.hpp"

class Gateway: public BinarySearchVector::ElementTemplate<Gateway, EuiType>
{
private:
	class Timer
	{
	private:
		uint64					lastTrueReturn_ms;	//time of last call to Tick() that returned true
		const uint64			duration_ms;	// period between successive time outs in ms

	public:
		Timer(uint64 myDuration_ms)
			: lastTrueReturn_ms(0), duration_ms(myDuration_ms)
		{}

		bool Tick() //returns true 'duration_ms' after last call that returned true
		{
			uint64 now_ms = GetMsSinceStart();
			
			if ((lastTrueReturn_ms + duration_ms) > now_ms)
				return false;

			Reset();
			return true;
		}
		void Reset() {lastTrueReturn_ms = GetMsSinceStart();}
	};

public:
	static const uint16 maxNumberOfGPSReadingsUsedToFindAverage;
	static const uint16 minNumberOfGPSReadingsUsedToFindAverage;
	static const sint16 defaultTransmitPower_dBm;
	static const sint16 maxTransmitPower_dBm;
	static const uint16 defaultDelayToNetworkServer_ms;
	static const uint16 defaultDelayFromNetworkServer_ms;
	static const uint64 nvStorageUpdatePeriod_ms = 60 * 1 * 1000;	//time between store operations in ms
	static const uint64 nvStatusClientUpdatePeriod_ms = 60 * 1 * 1000;	//time between sending status in ms

private:
	LoRa::Region				region;
	sockaddr_in					pullIpAddress;
	Position					configuredPosition;
	bool						allowGpsToSetPosition;
	ConsolidatedPosition		gpsPosition;
	GatewayFrameCountType		frameCount;
	Timer						nvStoreTimer;
	Timer						statusSendTimer;

public:
	Gateway(EuiType eui, LoRa::Region myRegion, Position const& myPosition, bool myAllowGpsToSetPosition)
		:
		BinarySearchVector::ElementTemplate<Gateway, EuiType>(eui),
		region(myRegion),
		configuredPosition(myPosition),
		allowGpsToSetPosition(myAllowGpsToSetPosition),
		gpsPosition(maxNumberOfGPSReadingsUsedToFindAverage, minNumberOfGPSReadingsUsedToFindAverage),
		nvStoreTimer(nvStorageUpdatePeriod_ms), statusSendTimer(nvStatusClientUpdatePeriod_ms)
	{
		SetInvalid(pullIpAddress);
	}

	sockaddr_in const& PullIpAddress() const {return pullIpAddress;}
	void PullIpAddress(sockaddr_in const& myPullIpAddress) {pullIpAddress = myPullIpAddress;}

	void Seen();
	void StatusReceived(uint64 receiveTime, GatewayStatusType const& newStatus);
	void SetRegion(LoRa::Region r);
	LoRa::Region Region() const {return region;}
	void AllowGpsToSetPosition(bool allow);
	bool AllowsGpsToSetPosition() const {return allowGpsToSetPosition;}
	void SetConfiguredPosition(Position const& p);
	bool PositionValid() const {configuredPosition.Valid() || (AllowsGpsToSetPosition() && gpsPosition.Valid());}
	Position const& ConfiguredPosition() const;
	Position const& GetPosition() const {return (AllowsGpsToSetPosition() && gpsPosition.Valid()) ? gpsPosition.Value() : configuredPosition;}

	bool TransmitFrame(WaitingFrame const& frame, uint32 gatewayReceiveTimestamp_us, MoteTransmitRecord const& upstreamFrameTransmit, uint8 transmissionWindow, bool isARepeatTransmission);

	//redefined virtual functions
	void CreateNVRecord() const;

private:
	void SendStatusToClients();

	void FormJsonFrameTransmitString(JSON::String& output, WaitingFrame const& frame, uint32 gatewayReceiveTimestamp_us, MoteTransmitRecord const& upstreamFrameTransmit, uint8 transmissionWindow) const;
	// gatewayReceiveTimestamp_us will frequently rollover
};


class GatewayList : public BinarySearchVectorNV::List<Gateway, EuiType>
{
public:
	GatewayList(bool initialise);
	Gateway* GetById(EuiType eui) const {return static_cast<Gateway*>(BinarySearchVectorNV::List<Gateway, EuiType>::GetById(eui));} 	//Gateway is locked
	Gateway* GetByIndex(uint32 index) const {return static_cast<Gateway*>(BinarySearchVectorNV::List<Gateway, EuiType>::GetByIndex(index));} 	//Gateway is locked

	void Add(EuiType eui, LoRa::Region region);
	void Add(EuiType eui, LoRa::Region myRegion, Position const& myPosition, bool myAllowGpsToSetPosition);

	void GatewaySeen(EuiType eui, sockaddr_in const* myPullIpAddress = 0);
	LoRa::Region GetRegion(EuiType eui) const;

	sockaddr_in FindPullIpAddress(EuiType eui) const
	{
		Gateway* gateway = GetById(eui);

		if (!gateway)
			return IP::nullAddress;

		//gateway is locked
		sockaddr_in result = gateway->PullIpAddress();
		gateway->Unlock();

		return result;
	}

	void StatusReceived(EuiType eui, uint64 receiveTime, GatewayStatusType const& newStatus) const
	{
		if ((eui == invalidEui) || (eui == nullEui))
			return;	//invalid (null) gateway - should never happen but avoids updating a non-existent record

		//lint --e{1774}  (Info -- Could use dynamic_cast to downcast polymorphic type
		Gateway* gateway = GetById(eui);

		if (!gateway)
			return;

		gateway->StatusReceived(receiveTime, newStatus);
		gateway->Unlock();
	}
};

//lint -e{1762} Info -- Member function 'BinarySearchVectorDebug::ElementTemplate<Gateway, EuiType>::TemplatedElementTick(void)' could be made const)
namespace BinarySearchVector
{
	template<> inline void ElementTemplate<Gateway, EuiType>::TemplatedElementTick(void)
	{
	}
}


#endif

