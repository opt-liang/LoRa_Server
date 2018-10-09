#ifndef REMINDER_HPP
#define REMINDER_HPP

#include "Eui.hpp"
#include "TimedQueueTemplate.hpp"

namespace EuiReminderQueue
{
	class Reminder : public TimedQueueTemplate::ElementParent
	{
	private:
		EuiType const		eui;
		uint16 const		token;	//16 bits of extra data

	public:
		static uint16 const invalidToken = ~0;

		Reminder(EuiType myEui, uint16 myToken, uint64 myTargetTime_ms)
			:TimedQueueTemplate::ElementParent(myTargetTime_ms), eui(myEui), token(myToken)
		{}

		Reminder(Reminder const& other)
			:TimedQueueTemplate::ElementParent(other.TargetTime_ms()), eui(other.Eui()), token(other.Token())
		{}

		Reminder()	//needed only for invalid value
			:TimedQueueTemplate::ElementParent(invalidTime), eui(invalidEui), token(0)
		{}

		EuiType Eui() const {return eui;}
		uint16 Token() const {return token;}
	};


	class Queue : public TimedQueueTemplate::List<Reminder>
	{
	public:
		Queue(void)
		{}

		void Add(EuiType eui, uint16 token, uint64 targetTime_ms, bool ensureUnique = true)	// ask to be reminded at targetTime_ms.  If onlyReminder is true, all other reminders for this eui and token will be deleted
		{
			Reminder reminder(eui, token, targetTime_ms);

			if (ensureUnique)
				AddUnique(reminder);
			else
				TimedQueueTemplate::List<Reminder>::Add(reminder);
		}

		Reminder Tick(uint64 now_ms) {return static_cast<Reminder>(TimedQueueTemplate::List<Reminder>::Tick(now_ms));}

	private:
		void AddUnique(Reminder const& reminder) //use instead of TimedQueueTemplate::List<Reminder>::AddQ(Reminder const& reminder)
		{
			//adds to list but deletes any element with the same Eui() and Token();
			MutexHolder holder(mutex);

			bool inserted = false;
			ListType::iterator it = list.begin();

			for (;it != list.end(); ++it)
			{
				//delete if the new entry matches this one
				while ((it->Eui() == reminder.Eui()) && (it->Token() == reminder.Token()))
				{
					it = list.erase(it);	//returns next element

					if (it == list.end())
						break;	//last element deleted
				}
				if (it == list.end())
					break;	//last element deleted

				if (!inserted && (it->TargetTime_ms() > reminder.TargetTime_ms()))	//list time greater than newElement time, so element must be inserted earlier
				{
					list.insert(it, reminder);	//insert before
					--it;	//to move the iterator to point at the newly inserted element, not the one after it
					inserted = true;
				}
			}

			if (!inserted)
				list.push_back(reminder);
		}
	};
}

#endif

