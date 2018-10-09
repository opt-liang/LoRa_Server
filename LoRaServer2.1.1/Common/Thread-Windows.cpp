/* © 2015 Semtech Corporation, 200 Flynn Road, Camarillo, CA 93012 U.S.A. All rights reserved.
The copyright of this file is owned by Semtech Corporation (Semtech). This is an unpublished work.  The content of this file must be used only for the purpose for which it was  supplied by Semtech or its distributors. The content of this file must not be copied or disclosed unless authorized in writing by Semtech.  */

#include "Thread.hpp"
#include <windows.h>



sint32 Thread::Run(void* myParameter)
{
	parameter = myParameter;

	UINT (*windowsFunction)  (LPVOID  ) 
			= (UINT (* ) (LPVOID  )) Thread::StartFunction;
	msThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)windowsFunction, this, 0, 0);

	sint32 result;
	if (msThread != 0)
		result = 0;
	else
		result = sint32(GetLastError());

	return result;
}

void Thread::Terminate()
{
	TerminateThread((HANDLE *) msThread, 0);
}

