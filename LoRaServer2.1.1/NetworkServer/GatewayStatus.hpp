/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef GATEWAY_STATUS_HPP
#define GATEWAY_STATUS_HPP

#include "TimeRecord.hpp"
#include "ValueWithValidity.hpp"
#include "Position.hpp"

struct GatewayFrameCountType
{
	ValidValueUint32		upstreamPacketsReceived;
	ValidValueUint32		upstreamGoodPacketsReceived;
	ValidValueUint32		upstreamPacketsForwarded;
	ValidValueUint32		upStreamDatagramsAcknowledged;
	ValidValueUint32		downstreamDatagramsReceived;
	ValidValueUint32		packetsTransmitted;

	GatewayFrameCountType() {}

	GatewayFrameCountType& operator+=(GatewayFrameCountType const& other)
	{
		upstreamPacketsReceived			+= other.upstreamPacketsReceived;
		upstreamGoodPacketsReceived		+= other.upstreamGoodPacketsReceived;
		upstreamPacketsForwarded		+= other.upstreamPacketsForwarded;
		upStreamDatagramsAcknowledged	+= other.upStreamDatagramsAcknowledged;
		downstreamDatagramsReceived		+= other.downstreamDatagramsReceived;
		packetsTransmitted				+= other.packetsTransmitted;
		return *this;
	}
};

struct GatewayStatusType
{
	TimeRecord					time;
	GatewayFrameCountType		count;
	Position					position;

	GatewayStatusType() {}
};

#endif

