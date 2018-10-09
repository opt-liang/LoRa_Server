/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef MUTEX_HPP
#define MUTEX_HPP

#ifdef _MSC_VER
	#include <ws2tcpip.h>
#else
	#include <pthread.h>
#endif
//#define DB_MUTEX
//http://msdn.microsoft.com/en-us/library/ms686927(v=vs.85).aspx
class Mutex
{
private:
#ifdef _MSC_VER
		//lint --e{1540}  Warning -- Pointer member 'Mutex::windowsMutex' (line 16) neither freed nor zeroed by destructor)
	HANDLE windowsMutex;
#else
	pthread_mutex_t posixMutex;
#endif

#ifdef DB_MUTEX
	int		lockCount;	// number of threads holding a lock on this mutex (should always be 1 or 0)
public:
	int LockCount() const {return lockCount;}
#endif
public:
	Mutex()
#ifdef _MSC_VER
		: windowsMutex(CreateMutex( 
		NULL,				// default security attributes
		FALSE,				// initially not owned
		NULL))				// unnamed mutex
#endif

	{
#ifdef DB_MUTEX
		lockCount = 0;
#endif
#ifndef _MSC_VER
		pthread_mutex_init(&posixMutex, NULL);
#endif
	}

	~Mutex() 
	{
		Lock();	//just to ensure any existing operation is complete
		Unlock();	//In Linux calling pthread_mutex_destroy() on a locked mutex results in undefined operation
#ifdef _MSC_VER
		CloseHandle(windowsMutex);
#else
		pthread_mutex_destroy(&posixMutex);
#endif
	}
	void Lock() 
	{
#ifdef _MSC_VER
		WaitForSingleObject( 
			windowsMutex,    // handle to mutex
			INFINITE);  // no time-out interval
#else
		pthread_mutex_lock(&posixMutex);
#endif
#ifdef DB_MUTEX
		lockCount++;
#endif
	}

	void Unlock() 
	{
#ifdef DB_MUTEX
		lockCount--;
#endif
#ifdef _MSC_VER
		ReleaseMutex(windowsMutex);
#else
		pthread_mutex_unlock(&posixMutex);
#endif
	}
};

class MutexHolder
{
private:
	Mutex& mutex;

public:
	MutexHolder(Mutex& myMutex)
		: mutex(myMutex)
	{mutex.Lock();}

	~MutexHolder() {mutex.Unlock();}
};


#endif 

