/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef MUTEX_DEBUG_HPP
#define MUTEX_DEBUG_HPP

#include "Mutex.hpp"
#include "DebugMonitor.hpp"
#include <string>
#include <sstream>
#include <stack>

class MutexDebug : public Mutex
{
private:
	std::string					name;
	std::stack<std::string>		stack;
public:
	MutexDebug(char const myName[] = "")
		: name(myName)
	{}

	~MutexDebug() {}

	void Lock(std::string const& locker)
	{
		std::stringstream logText;
		logText << "Locking " << name << " " << locker << " Count = " << LockCount();
		Debug::Write(logText);

		Mutex::Lock();
		logText.str();
		logText << "Locked " << name << " " << locker << " Count = " << LockCount();
		Debug::Write(logText);

		if (stack.size() != 0)
		{
			std::stringstream errorText;

			errorText << "ERROR 2nd lock " << locker << "  " << stack.top();
			Debug::Write(errorText);
		}
		stack.push(locker);
	}
	void Unlock()
	{
		std::stringstream logText;
		logText << "Unlocking " << name << " " << stack.top() << " Count = " << LockCount();
		Debug::Write(logText);

		if (stack.size() != 1)
			Debug::Write("ERROR 2nd unlock");
		stack.pop();
		Mutex::Unlock();
	}
};

class MutexHolderDebug
{
private:
	std::string name;
	MutexDebug&	mutex;
public:
	MutexHolderDebug(MutexDebug& myMutex, std::string const& myName)
		:name(myName), mutex(myMutex)
	{
		mutex.Lock(std::string("Holder ") + name);
	}

	~MutexHolderDebug() {mutex.Unlock();}
};


#endif 

