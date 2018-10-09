/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef JOIN_CONTROLLER_HPP
#define JOIN_CONTROLLER_HPP

#include "General.h"
#include "LoRa.hpp"
#include "LoRaReceiveFrame.hpp"
#include "BestGateway.hpp"
#include "AddressAllocator.hpp"
#include "Mutex.hpp"
#include "ValueWithValidity.hpp"
#include "BinarySearchVector.hpp"

#include <list>

class JoinController
{
	class Request : public BinarySearchVector::ElementTemplate<Request, EuiType>
	{
	public:
		friend class JoinController;

	private:
		static const uint16		tickPeriod_ms = 1000;

		JoinController&			controller;
		ValidValueUint32		moteNetworkAddress;	//only valid after acceptance
		EuiType					appEui;
		uint64					serverReplyTime_ms;
		BestGateway				bestGateway;
		uint16					deviceNonce;

		Request(JoinController& myController, LoRa::ReceivedFrame const& frame, uint64 myServerReplyTime_ms);
		~Request()
		{
			Lock();
			if (moteNetworkAddress.Valid())
				controller.ReleaseNetworkAddress(moteNetworkAddress);
			Unlock();	//prior to being locked by ElementTemplate destructor
		}

		void ReceivedRequest(LoRa::ReceivedFrame const& frame, bool firstCopy);	//from mote - firstCopy is true if this is the first copy of this particular request

	public:
		void ReceivedDuplicateRequest(LoRa::ReceivedFrame const& frame) {ReceivedRequest(frame, false);}
		void ReceivedAccept(bool accept); //from application server
		void ReceivedComplete(LoRa::Frame const& frame, LoRa::CypherKey const& networkKey); //from application server - then deleted
		EuiType MoteEui() const {return Id();}
		EuiType AppEui() const {return appEui;}
		uint16 DeviceNonce() const {return deviceNonce;}
	};

	BinarySearchVector::List<Request, EuiType>			list;
	Mutex												mutex;
	AddressAllocator									addressAllocator;

public:
	JoinController(uint32 myListTickPeriod_ms)
		: list(myListTickPeriod_ms, JoinController::Request::tickPeriod_ms, 2 * LoRa::moteJoinRequestWindowTime_us / 1000, true)
		//Multiplication by 2 is done to give a safety margin - the tick rate is inaccurate
	{}
	~JoinController(void) {}
	void DeleteAllElements();

	bool IsInitialised() const {return addressAllocator.IsSet();}

	void ReceivedRequest(LoRa::ReceivedFrame const& frame);
	void ReceivedAccept(EuiType moteEui, bool accept);
	void ReceivedComplete(EuiType moteEui, LoRa::Frame const& frame, LoRa::CypherKey const& networkKey);

	void Tick() {list.Tick();}

	bool ReserveNetworkAddress(EuiType moteEui, uint32 networkAddress) {return addressAllocator.ReserveNetworkAddress(moteEui, networkAddress);}

	void ReleaseNetworkAddress(uint32 networkAddress) {addressAllocator.Release(networkAddress);}

	void SetNetworkId(uint16 id) {addressAllocator.SetNetworkAddressRange(id << LoRa::networkAddressBits, LoRa::networkAddressMask);}
	EuiType FindEui(uint32 networkAddress) {return addressAllocator.FindEui(networkAddress);}
};

//lint -e{1762} Info -- Member function 'BinarySearchVectorDebug::ElementTemplate<Gateway, EuiType>::TemplatedElementTick(void)' could be made const)
namespace BinarySearchVector
{
	template<> inline void ElementTemplate<JoinController::Request, EuiType>::TemplatedElementTick(void)
	{
	}
}
#endif
