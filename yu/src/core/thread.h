#ifndef YU_THREAD_H
#define YU_THREAD_H

#include "platform.h"
#if defined YU_OS_WIN32
	#include <windows.h>
#elif defined YU_OS_MAC
	#include <mach/mach.h>
	#include <pthread.h>
#endif

namespace yu
{

#if defined YU_OS_WIN32
	typedef DWORD ThreadReturn;
	typedef PVOID ThreadContext;
	#define ThreadCall WINAPI
#elif defined YU_OS_MAC
	typedef void* ThreadReturn;
	typedef void* ThreadContext;
	#define ThreadCall 
#endif

typedef ThreadReturn(ThreadCall* ThreadFunc)(ThreadContext);

enum ThreadPriority
{
	CriticalPriority,
	HighestPriority,
	AboveNormalPriority,
	NormalPriority,
	BelowNormalPriority,
	LowestPriority,
	IdlePriority,
};

struct Thread
{
#if defined YU_OS_WIN32
	HANDLE	hThread;
	DWORD	threadId;
#else
	pthread_t hThread;
#endif
	
};

Thread	CreateThread(ThreadFunc func, ThreadContext context, ThreadPriority priority = NormalPriority, u64 affinityMask = 0);
u64		GetThreadAffinityMask(Thread& thread);
void	SetThreadAffinity(Thread& thread, u64 affinityMask);
void	SetThreadName(Thread& thread, const char* name);
void	SetThreadPriority(Thread& thread, ThreadPriority priority);
}

#endif
