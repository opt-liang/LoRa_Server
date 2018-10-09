/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef BINARY_SEARCH_VECTOR_DEBUG_HPP
#define BINARY_SEARCH_VECTOR_DEBUG_HPP

#include "General.h"
#include "MutexDebug.hpp"
#include "Utilities.hpp"

#include <vector>
#include <string>

/*Each class ElementType must inherit from either BinarySearchVector::ElementTemplate */

namespace BinarySearchVectorDebug
{
	typedef uint32 IndexType;
	const uint32 forever = ~uint32(0);
	//lint --e{1795} (Info -- defined but not instantiated)

	template <class ElementType, class IdType> class ElementTemplate //allows comparison functions to be redefined
	{
	protected:
		IdType				id;
		uint32				tickCount;
		bool				requestingDeletion;	//only useful when List is ticked
		mutable MutexDebug	mutex;

		ElementTemplate(char const myName[], IdType myId) : id(myId), tickCount(0), requestingDeletion(false), mutex(myName) {}

	public:
		//lint -e{1551} Warning -- Function may throw exception '...'
		virtual ~ElementTemplate() {}

		IdType Id() const {return id;}

		uint32 Tick()	//returns ticks since ResetTimeToLive() called
		{
			TemplatedElementTick();
			if (tickCount != forever)
				tickCount++;

			return tickCount;
		}

		void ResetTimeToLive() {tickCount = 0;}

		void RequestDeletion() {requestingDeletion = true;}
		void CancelAnyPendingDeletionRequest() {requestionDeletion = false;}
		bool RequestingDeletion() const {return requestingDeletion;}

		void Lock(std::string const& caller) const {mutex.Lock(caller);}
		void Unlock() const {mutex.Unlock();}

	protected:
		//must be defined for each template type
		void TemplatedElementTick();
	};

	template<class ElementType, class IdType> class List
	{
	private:
		IndexType			nextTickedIndex;	//next index to be ticked
		const uint32		listTickPeriod_ms;
		const uint32		elementTickPeriod_ms;
		const uint32		elementTimeout_ticks;
		uint32				tickCounter;	// used when Tick() returns early due to requiring an external response to timeout
		bool				deleteInternally;	//inside Tick() function, if false Id of an element to be deleted will be returned

		//following attributes set by ChangeTickCycle()
		uint32				elementsPerTick;
		uint32				ticksAtStartOfCycle;	// list ticks when nothing happens - to even tick rate
		uint32				startOfCycleTickCount;

	protected:
		mutable MutexDebug		mutex;

	public:
		typedef ElementTemplate<ElementType, IdType> Element;

		List(char const myName[] = "")	// not ticked
			: nextTickedIndex(0),
			listTickPeriod_ms(0),
			elementTickPeriod_ms(0),
			elementTimeout_ticks(forever),
			tickCounter(0),
			deleteInternally(false),

			//these values are not used until initialised by ChangeTickCycle() - set here to avoid compiler warnings
			elementsPerTick(1),
			ticksAtStartOfCycle(0),
			startOfCycleTickCount(1),
			mutex(myName)
		{}


		List(char const myName[], uint32 myListTickPeriod_ms, uint32 myElementTickPeriod_ms, uint32 myElementTimeout_ms, bool myDeleteInternally) 	// myElementTickPeriod_ms must not be zero
			: nextTickedIndex(0),
			listTickPeriod_ms(myListTickPeriod_ms),
			elementTickPeriod_ms(myElementTickPeriod_ms),
			elementTimeout_ticks(myElementTimeout_ms/myElementTickPeriod_ms + 1),	//round up
			tickCounter(0),
			deleteInternally(myDeleteInternally),

			//these values are not used until initialised by ChangeTickCycle() - set here to avoid compiler warnings
			elementsPerTick(1),
			ticksAtStartOfCycle(0),
			startOfCycleTickCount(1),
			mutex(myName)
		{}

		//lint -e{1551} Warning -- Function may throw exception '...'
		virtual ~List()
		{
			//lint -e{1551} (Warning -- Function may throw exception '...' in destructor 'BinarySearchVector::List<unsigned int>::~List(void)')
			DeleteAllElements();
		}

		Element* GetById(IdType id) const	//returned element is locked
		{
			MutexHolder holder(mutex);
			return GetByIdPrivate(id);
		}

		Element* GetByIndex(IndexType index) const 	//returned element is locked
		{
			MutexHolderDebug holder(mutex, "GetByIndex");
			return GetByIndexPrivate(index);
		}

		IndexType Add(Element* element)
		{
			MutexHolderDebug holder(mutex, "Add");
			return AddPrivate(element);
		}

		bool DeleteById(IdType id)
		{
			MutexHolder holder(mutex, "DeleteById");
			Element* element = RemoveByIdPrivate(id);	//element is locked

			if (!element)
				return false;

			element->Unlock();
			delete element;
			return true;
		}

		bool DeleteByIndex(IndexType index)
		{
			MutexHolderDebug holder(mutex, "DeleteByIndex");
			Element* element = RemoveByIndexPrivate(index);	//element is locked

			if (!element)
				return false;

			element->Unlock();
			delete element;
			return true;
		}

		bool Exists(IdType id) const //returns true if key equals one element
		{
			Element* element = GetById(id);

			if (element)
				element->Unlock();

			return element != 0;
		}

		IndexType Size() const {return IndexType(store.size());}
		bool IsEmpty() const {return (Size() == 0) ? true : false;}
		bool DeleteInternally() const {return deleteInternally;}

		//tick should be called with a regular period of myListTickPeriod_ms
		//if the function returns with a value other than ~IdType(0), the function should be called again to ensure correct timing

		//If deleteInternally is true, deletes any timed out element.  Return value is always ~IdType(0)
		//If deleteInternally is false, returns the Id of an element that must be deleted.  


		IdType Tick(bool print = false)
		{
			if (!Tickable())	// no need for mutex because Tickable() result does not change
				return ~IdType(0);

			MutexHolder holder(mutex);

			if (print)
				Debug::Write(std::string("Size ") + ConvertIntToText(Size()));

			if (Size() == 0)
				return ~IdType(0);

			if (startOfCycleTickCount > 0)
			{
				if (print)
					Debug::Write(std::string("Start of cycle tick " + ConvertIntToText(startOfCycleTickCount)));
				startOfCycleTickCount--;	//do nothing for first 'ticksAtStartOfCycle' ticks
			}
			else if (nextTickedIndex < Size())
			{
				IndexType end = elementsPerTick + nextTickedIndex;
				for (; tickCounter < end && nextTickedIndex < Size(); tickCounter++)
				{
					Element* element = GetByIndexPrivate(nextTickedIndex);	//element is locked
					uint32 ticks = element->Tick();

					if (!element)
					{
						if (Debug::Print(Debug::major))
							Debug::Write("Unexpected null pointer in BinarySearchVector::List::Tick() 1");
						continue;
					}

					if (print)
					{
						std::stringstream text;

						text << "Tick element " << AddressText(element->Id()) << " ticks " << ticks <<
						" Index = " << (nextTickedIndex - 1);
					
						Debug::Write(text);
					}

					bool timeout = ticks >= elementTimeout_ticks;
					bool deleteElement = timeout || element->RequestingDeletion();

					element->Unlock();
					element = 0;

					if (deleteElement)
					{
						if (deleteInternally)
						{
							element = RemoveByIndexPrivate(nextTickedIndex);	//element is locked
							end--;	// don't increment index - all higher elements have moved down by 1

							if (!element)
							{
								if (Debug::Print(Debug::major))
									Debug::Write("Unexpected null pointer in BinarySearchVector::List::Tick() 2");

								continue;
							}

							//removedElement can no longer be found by another thread as is not already locked by one
							element->Unlock();	//unlock prior to deletion

							delete element;
							element = 0;
						}
						else
							return element->Id();
					}
					else
						nextTickedIndex++;
				}
				tickCounter = 0;
			}
			else if (startOfCycleTickCount == 0 && nextTickedIndex >= Size())
			{
				//end of cycle
				nextTickedIndex = 0;
				startOfCycleTickCount = ticksAtStartOfCycle;
			}
			return ~IdType(0);
		}

		IdType FindNextFreeId(IdType min) const
		{
			MutexHolder holder(mutex);
			
			uint32 size = Size();
			if (size == 0)
				return min;

			IdType nextId = min;
			IndexType index = Find(min);
			while (index < size && store[index]->Id() == nextId)
			{
				nextId++;
				index++;
			}

			if (index >= size)
				nextId = store[size - 1]->Id() + 1;

			return nextId;
		}

		void DeleteAllElements()
		{
			while (!store.empty())
				DeleteByIndex(0);
		}

	protected:
		std::vector<ElementTemplate<ElementType, IdType>*> store;

		uint32 AddPrivate(Element* element)
		{
			IndexType index = Find(element->Id());
	
			if (index >= store.size())
				store.push_back(element);
			else
			{
				if (store[index]->Id() == element->Id())
				{
					//Id() is already in store
					// delete old element

					store[index]->Lock();	//wait for any locks to be released
					store[index]->Unlock();	//release before deletion
					delete store[index];	//delete pointed to element, not store location

					store[index] = element;
				}
				else
				{
					store.insert(store.begin() + int(index), element); //not already in array

					if (index >= nextTickedIndex)
						nextTickedIndex++;	// to avoid ticking the same element twice (or immediately ticking the new one)
				}
			}

			ChangeTickCycle();
			return index;
		}

		Element* GetByIdPrivate(IdType id) const	//returned element is locked
		{
			uint32 index = Find(id);

			Element* ptr = 0;

			if (index < store.size() && store[index]->Id() == id)
			{
				ptr = store[index];
				ptr->Lock("GetByIdPrivate");
			}
			return ptr;
		}

		Element* GetByIndexPrivate(IndexType index) const 	//returned element is locked
		{
			Element* ptr = 0;
			if (index < store.size())
			{
				ptr = store[index];
				ptr->Lock("GetByIndexPrivate");
			}

			return ptr;
		}

		Element* RemoveByIdPrivate(IdType id)	//removes from list and returns (locked) pointer
		{
			IndexType index = Find(id);

			if ((index >= store.size()) || (id != store[index]->Id()))
				return false;

			return RemoveByIndexPrivate(index);
		}

		Element* RemoveByIndexPrivate(IndexType index)	//removes from list and returns (locked) pointer
		{
			if (index >= store.size())
				return 0;

			Element* element = store[index];

			if (element)
				element->Lock();

			//lint -e{1702} (Info -- operator 'operator+' is both an ordinary function and a member function
			store.erase(store.begin() + int(index));

			if (index < nextTickedIndex)
				nextTickedIndex--;	//to avoid missing an element's tick

			ChangeTickCycle();

			return element;
		}

		IndexType Find(IdType targetId) const	//returns location or otherwise location to insert
		{
			if (store.empty())
				return 0;

			IndexType base = 0;
			IndexType top = store.size() - 1;

			while((top - base) > 1)
			{
				// search between top and base
				IndexType testIndex = (top - base) / 2 + base;	//pick the midway point

				ElementType const* test = (ElementType const*) store[testIndex];

				if (test->Id() == targetId)
					return testIndex;	//found exact value

				else if (test->Id() > targetId)
					top = testIndex; //target is below the halfway point

				else if (test->Id() < targetId)
					base = testIndex;	//target is above the halfway point
			}

			//base and top are either equal or one different
			if (targetId <= store[base]->Id())
				return base;

			else if (targetId <= store[top]->Id())
				return top;
			else
				return top + 1;
		}

		
		bool Consistent() const
		{
			if (store.empty())
				return true;

			IdType oldId = store[0]->Id();
			IndexType size = store.size();

			for (IndexType i = 1; i < size; i++)
			{
				IdType test = store[i]->Id();

				if (oldId >= test)
					return false;

				oldId = test;
			}
			return true;
		}

	private:
		bool Tickable() const {return listTickPeriod_ms != 0;}

		void ChangeTickCycle()
		{
			//lint --e{414} (Warning -- Possible division by 0) - listTickPeriod_ms - but impossible
			if (!Tickable())
				return;

			uint size = Size();

			if (size == 0)
				return; //nothing to be done

			uint cyclePeriod_ticks = elementTickPeriod_ms / listTickPeriod_ms; //round down

			if (cyclePeriod_ticks > 1)
			{
				elementsPerTick = size / cyclePeriod_ticks + ((size % cyclePeriod_ticks) ? 1 : 0); // round up;
				ticksAtStartOfCycle = cyclePeriod_ticks - size * elementsPerTick;
			}
			else
			{
				//the element tick period is no longer than the list period, so tick all the elements each time
				elementsPerTick = size;	// all the elements are 
				ticksAtStartOfCycle = 0;
			}
		}
	};
}

#endif

