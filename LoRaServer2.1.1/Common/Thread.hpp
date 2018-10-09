/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#ifndef THEAD_H
#define THEAD_H
#include "General.h"

#ifdef _MSC_VER
	#define THREAD_RUN_RETURN_TYPE void
#else
	#include <pthread.h>
	#define THREAD_RUN_RETURN_TYPE void*
#endif

class Thread
{
private:
	const uint	stackSize;
	void*		parameter;
#ifdef _MSC_VER
	void* msThread;		//Defined as type HANDLE in Windows.h (not included)
#else
	pthread_t posix_thread;
#endif

	void (*initialiseFunction) () ;
	THREAD_RUN_RETURN_TYPE (*runFunction) (void*) ;

	static void StartFunction(void* threadptr);	// first parameter is class pointer
public:
	Thread(THREAD_RUN_RETURN_TYPE (*aRunFunction) (void*) = 0, uint myStackSize = 1024 * 1024)
	: stackSize(myStackSize), parameter(0),
#ifdef _MSC_VER
	msThread(0),
#endif
	initialiseFunction(0), runFunction(aRunFunction)
	{}

	Thread(void (*aInitialiseFunction) (), THREAD_RUN_RETURN_TYPE (*aRunFunction) (void*), uint myStackSize = 16384)
		: stackSize(myStackSize), parameter(0),
#ifdef _MSC_VER
	msThread(0),
#endif
		initialiseFunction(aInitialiseFunction), runFunction(aRunFunction)
	{}
	sint32 Run(void* myParameter = 0); // returns 0 on success, otherwise the error number
	void RunFunction(THREAD_RUN_RETURN_TYPE (*aRunFunction) (void*)) {runFunction = aRunFunction;}

	void* GetParameter() {return parameter;}

	void Terminate();
};

inline void Thread::StartFunction(void * threadptr)
{
	Thread* thread = (Thread *) threadptr;

	if (thread->initialiseFunction)
		thread->initialiseFunction();

	thread->runFunction(thread->GetParameter());
}
#endif


