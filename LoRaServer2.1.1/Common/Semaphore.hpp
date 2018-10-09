/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef SEMAPHORE_HPP
#define SEMAPHORE_HPP


#ifdef _MSC_VER
	#include <windows.h>
#else
	#include <semaphore.h>
#endif

class Semaphore
{
private:
#ifdef _MSC_VER
	HANDLE handle;
#else
	sem_t	sem;
#endif

public:
	Semaphore(unsigned maxCount = 1, unsigned startValue = 0)
	{
#ifdef _MSC_VER
		handle = CreateSemaphore(
			0,           // default security attributes
			startValue,  // initial count
			maxCount,    // maximum count
			0);          // unnamed semaphore
#else
		sem_init(&sem, 0, startValue);
#endif
	}

	~Semaphore(void)
	{
#ifdef _MSC_VER
		CloseHandle(handle);
#else
		sem_destroy(&sem);
#endif
	}

	void Wait()
	{
#ifdef _MSC_VER
	int dwWaitResult = WaitForSingleObject( 
		handle,   // handle to semaphore
		INFINITE);           // no time-out interval

#else
		sem_wait(&sem);
#endif
	}

	void Post()
	{
#ifdef _MSC_VER
		ReleaseSemaphore( 
			handle,  // handle to semaphore
			1,            // increase count by one
			NULL);      // not interested in previous count
#else
		sem_post(&sem);
#endif
	}

};

#endif
