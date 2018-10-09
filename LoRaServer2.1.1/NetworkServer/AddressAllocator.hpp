/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef ADDRESS_ALLOCATOR_HPP
#define ADDRESS_ALLOCATOR_HPP

#include "Eui.hpp"
#include "LoRa.hpp"
#include "BinarySearchVector.hpp"
#include "AddressRange.hpp"
#include "Mutex.hpp"

class AddressAllocator
{
private:
	class Address : public BinarySearchVector::ElementTemplate<Address, uint32>
	{
	private:
		EuiType			eui;
	public:
		Address(uint32 myNetworkAddress, EuiType myEui)
			: BinarySearchVector::ElementTemplate<Address, uint32>(myNetworkAddress), eui(myEui)
		{}

		EuiType Eui() const {return eui;}
	};

	class List : public BinarySearchVector::List<Address, uint32>
	{
	public:
		List()
		{}

		void AddToList(uint32 networkAddress, EuiType eui)
		{
			Address* add = new Address(networkAddress, eui);

			Add(add);
		}
	};

	List			list;
	Mutex			mutex;
	AddressRange	allocatedRange;
	uint32			firstFreeAddress;	//not initialsed until SetNetworkAddressRange called

public:
	AddressAllocator()
		:firstFreeAddress(LoRa::invalidNetworkAddress)
	{}

	//lint -e{1551} (Warning -- Function may throw exception '...' in destructor 'BinarySearchVector::List<unsigned int>::~List(void)')
	~AddressAllocator(void) {}

	void SetNetworkAddressRange(uint32 myBase, uint32 myMask) {allocatedRange.Set(myBase, myMask); firstFreeAddress = list.FindNextFreeId(allocatedRange.Base());}
	uint32 Request(EuiType eui)
	{
		if (firstFreeAddress > allocatedRange.Maximum())
			return LoRa::invalidNetworkAddress;

		MutexHolder holder(mutex);
		list.AddToList(firstFreeAddress, eui);
		uint32 result = firstFreeAddress;

		firstFreeAddress = list.FindNextFreeId(firstFreeAddress);

		return result;
	}

	void Reset()
	{
		MutexHolder holder(mutex);
		list.DeleteAllElements();
		firstFreeAddress = list.FindNextFreeId(allocatedRange.Base());
	}

	bool ReserveNetworkAddress(EuiType moteEui, uint32 networkAddress)
	{
		MutexHolder holder(mutex);

		if (Allocated(networkAddress))
		{
			if (Debug::Print(Debug::major))
			{
				std::stringstream text;

				text << "Address Allocator requested to reserve address " << AddressText(networkAddress) << " for mote " << AddressText(moteEui) << " already allocated to " << FindEuiPrivate(networkAddress);
				Debug::Write(text);
			}
			ReleasePrivate(networkAddress);	//new data is perferred over old data
		}

		list.AddToList(networkAddress, moteEui);

		if (networkAddress == firstFreeAddress)
			firstFreeAddress = list.FindNextFreeId(firstFreeAddress);

		return true;
	}


	void Release(uint32 networkAddress)
	{
		MutexHolder holder(mutex);
		ReleasePrivate(networkAddress);
	}

	EuiType FindEui(uint32 networkAddress)
	{
		MutexHolder holder(mutex);
		return FindEuiPrivate(networkAddress);
	}

	bool Allocated(uint32 networkAddress) {return list.Exists(networkAddress);}

	bool IsSet() const {return allocatedRange.IsSet();}

private:

	void ReleasePrivate(uint32 networkAddress)
	{
		if (!list.DeleteById(networkAddress))
			return;

		if (firstFreeAddress > networkAddress)
			firstFreeAddress = networkAddress;
	}

	EuiType FindEuiPrivate(uint32 networkAddress)
	{
		//lint --e{1774}  (Info -- Could use dynamic_cast to downcast polymorphic type ')
		Address* address = static_cast<Address*>(list.GetById(networkAddress));

		if (address)
		{
			EuiType result = address->Eui();

			address->Unlock();
			return result;
		}
		else
			return invalidEui;
	}
};

#endif
