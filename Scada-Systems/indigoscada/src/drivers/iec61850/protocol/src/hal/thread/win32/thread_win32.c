/*
 *  thread_win32.c
 *
 *  Copyright 2013 Michael Zillgith
 *
 *	This file is part of libIEC61850.
 *
 *	libIEC61850 is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	libIEC61850 is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with libIEC61850.  If not, see <http://www.gnu.org/licenses/>.
 *
 *	See COPYING file for the complete license text.
 */

#include <windows.h>
#include <stdlib.h>
#include "thread.h"

struct sThread {
	ThreadExecutionFunction function;
	void* parameter;
	HANDLE handle;
	int state;
	bool autodestroy;
};

static DWORD WINAPI
destroyAutomaticThreadRunner(LPVOID parameter)
{
	Thread thread = (Thread) parameter;

	thread->function(thread->parameter);

	Thread_destroy(thread);

	return 0;
}

static DWORD WINAPI
threadRunner(LPVOID parameter)
{
	Thread thread = (Thread) parameter;

	return (UINT) thread->function(thread->parameter);
}

Thread
Thread_create(ThreadExecutionFunction function, void* parameter, bool autodestroy)
{
	DWORD threadId;
	Thread thread = malloc(sizeof(struct sThread));

	thread->parameter = parameter;
	thread->function = function;
	thread->state = 0;
	thread->autodestroy = autodestroy;

	if (autodestroy == true)
		thread->handle = CreateThread(0, 0, destroyAutomaticThreadRunner, thread, CREATE_SUSPENDED, &threadId);
	else
		thread->handle = CreateThread(0, 0, threadRunner, thread, CREATE_SUSPENDED, &threadId);

	return thread;
}

void
Thread_start(Thread thread)
{
	thread->state = 1;
	ResumeThread(thread->handle);
}

void
Thread_destroy(Thread thread)
{
	if (thread->state == 1)
		WaitForSingleObject(thread->handle, INFINITE);

	CloseHandle(thread->handle);

	free(thread);
}

void
Thread_sleep(int millies)
{
	Sleep(millies);
}

