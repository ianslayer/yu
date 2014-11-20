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
	typedef HANDLE ThreadHandle;
#else
	typedef void* ThreadReturn;
	typedef void* ThreadContext;
	#define ThreadCall 
	typedef pthread_t ThreadHandle;
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
	ThreadHandle threadHandle;
};

void	InitThreadRuntime();
void	FreeThreadRuntime();
Thread	CreateThread(ThreadFunc func, ThreadContext context, ThreadPriority priority = NormalPriority, u64 affinityMask = 0);
ThreadHandle GetCurrentThreadHandle();
//u64		GetThreadAffinityMask(Thread& thread);
void	SetThreadAffinity(ThreadHandle threadHandle, u64 affinityMask);
void	SetThreadName(ThreadHandle thread, const char* name);
void	SetThreadPriority(ThreadHandle thread, ThreadPriority priority);
bool	AllThreadExited();
unsigned int	NumThreads();
struct  FrameLock;
FrameLock*	AddFrameLock(ThreadHandle handle);
void	WaitForKick(FrameLock* lock);
void	FrameComplete(FrameLock* lock);
void	DummyWorkLoad(double timeInMs);

class Mutex
{
public:
	Mutex();
	~Mutex();

	void Lock();
	void Unlock();
#if defined YU_OS_WIN32
	CRITICAL_SECTION m;
#else
	pthread_mutex_t m;
#endif
};

class ScopedLock
{
public:
	ScopedLock(Mutex& m);
	~ScopedLock();

	Mutex& m;
};

class CondVar
{
public:
	CondVar();
	~CondVar();

#if defined YU_OS_WIN32
	CONDITION_VARIABLE cv;
#else
	pthread_cond_t cv;
#endif
};

void WaitForCondVar(CondVar& cv, Mutex& m);
void NotifyCondVar(CondVar& cv);
void NotifyAllCondVar(CondVar& cv);

struct Event
{
	CondVar			cv;
	Mutex			cs;
	bool			signaled = false;
};

void WaitForEvent(Event& ev);
void SignalEvent(Event& ev);
void ResetEvent(Event& ev);

}

#endif
