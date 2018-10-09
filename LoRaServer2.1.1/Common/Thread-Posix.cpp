/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "Thread.hpp"

//
/* http://www.ibm.com/developerworks/linux/library/l-ipc2lin1.html */



// return true for success
int Thread::Run(void* myParameter)
{
	pthread_attr_t threadAttr;
	/* initialize the thread attribute */
	pthread_attr_init(&threadAttr);

	/* Set the stack size of the thread */
	pthread_attr_setstacksize(&threadAttr, stackSize);

	/* Set thread to detached state. No need for pthread_join */
	pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);

	return pthread_create(&posix_thread, &threadAttr, runFunction, myParameter);
}

void Thread::Terminate()
{
	pthread_cancel(posix_thread);
}


