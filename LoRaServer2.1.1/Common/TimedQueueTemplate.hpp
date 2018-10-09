/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef TIMED_QUEUE_TEMPLATE_HPP
#define TIMED_QUEUE_TEMPLATE_HPP

#include "General.h"
#include "Mutex.hpp"

#include <list>
#include <iterator>
#include <stddef.h>

namespace TimedQueueTemplate
{
	class ElementParent
	{
	private:
		uint64		targetTime_ms;	//in ms - value of GetMsSinceStart()

	public:
		ElementParent(uint64 myTargetTime_ms = invalidTime)
			:targetTime_ms(myTargetTime_ms)
		{}

		ElementParent& operator=(ElementParent const& other)
		{
			targetTime_ms = other.TargetTime_ms();
			return *this;
		}

		uint64 TargetTime_ms() const {return targetTime_ms;}	//in ms - value of GetMsSinceStart()

		bool Valid() const {return TargetTime_ms() != invalidTime;}
	};

	template <typename Element> class List	//Element must be a child of ElementParent
	{
	protected:
		typedef std::list<Element>	ListType;

		//lint -e{1509} (Warning -- base class destructor for class 'list' is not virtual)
		ListType					list;	//elements with earliest target time at the beginning
		mutable Mutex				mutex;
		Element						invalid;	//returned when no valid element exists

	public:
		List() {}

		~List()
		{
			MutexHolder holder(mutex);
			for(;!Empty();)
				RemoveNextPrivate();
		}

		Element Tick(uint64 now)	//returns next element if its target time is <= now
		{
			MutexHolder holder(mutex);

			if (Empty() || NextTargetTimePrivate_ms() > now)
				return invalid;

			return RemoveNextPrivate();
		}

		void Add(Element const& newElement)	// adds an element to the list
		{
			MutexHolder holder(mutex);

			typename ListType::reverse_iterator it = list.rbegin();	//rbegin is last element of the list

			//lint -e{1702} Info -- operator 'operator!=' is both an ordinary function and a member function
			for (;it != list.rend(); ++it)	// ++ moves reverse iterator 
			{
				if (it->TargetTime_ms() > newElement.TargetTime_ms())	//list time greater than newElement time, so element must be inserted earlier
					continue;

				list.insert(it.base(), newElement);
				return;
			}

			// insert at beginning of list
			list.push_front(newElement);
		}

		Element RemoveNext()
		{
			MutexHolder holder(mutex);
			return RemoveNextPrivate();
		}


		uint64 NextTargetTime_ms() const
		{
			MutexHolder holder(mutex);
			return NextTargetTimePrivate_ms();
		}

	private:
		Element const& Next() const {list.front();}

		bool Empty() const {return list.empty();}

		Element RemoveNextPrivate()	// must never be called when list is empty
		{
			Element result = list.front();
			list.pop_front();
			return result;
		}

		uint64 NextTargetTimePrivate_ms() const
		{
			if (list.empty())
				return invalidTime;

			return list.front().TargetTime_ms();
		}
	public:
		bool Consistent() const
		{
			MutexHolder holder(mutex);

			if (Empty())
				return true;

			typename std::list<Element>::const_iterator it = list.begin();	//rbegin is last element of the list

			uint64 old = 0;

			//lint --e{1702}  (Info -- operator 'operator!=' is both an ordinary function and a member function
			for (; it != list.end(); ++it)
			{
				uint64 next = it->TargetTime_ms();

				if (old > next)
					return false;

				old = next;
			}
			return true;
		}
	};
};

#endif

