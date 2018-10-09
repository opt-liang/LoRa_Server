/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef QUEUE_HPP
#define QUEUE_HPP

#include "Semaphore.hpp"
#include "Mutex.hpp"

#include <queue>

template <class ElementType> class QueueTemplate
{
protected:
	std::queue<ElementType*>		queue;
	uint const						maxLength;
	bool							deleteFromTail;	//if queue is full remove newest element (true) or oldest (false)
	mutable Semaphore				semaphore;
	mutable Mutex					mutex;
	
public:
	QueueTemplate(bool myDeleteFromTail = false, uint myMaxLength = ~(0))
		:deleteFromTail(myDeleteFromTail), maxLength(myMaxLength)
	{}

	//newElement cannot be used by calling function after Add is called
	bool Add(ElementType& newElement)
	{
		MutexHolder holder(mutex);
		bool success = true;
		if (Length() >= maxLength)
		{
			success = false;
			if (deleteFromTail)
				delete &newElement;
			else
			{
				ElementType* deletedElement = NextElement();

				delete deletedElement;
			}

			return success;
		}

		queue.push(&newElement);
		semaphore.Post();
		return success;
	}

	ElementType* Read()	//calling function must delete returned element
	{
		MutexHolder holder(mutex);

		while (queue.empty())
		{
			mutex.Unlock();
			semaphore.Wait();
			mutex.Lock();
		}

		ElementType* result = NextElement();

		return result;
	}

	uint Length() const {return queue.size();}

private:
	ElementType* NextElement()
	{
		if (queue.empty())
			return 0;

		ElementType* result = queue.front();
		queue.pop();
		return result;
	}
};

#endif