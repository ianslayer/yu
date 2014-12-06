#ifndef YU_THREAD_H
#define YU_THREAD_H

#include "platform.h"
#if defined YU_OS_WIN32
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <windows.h>
#elif defined YU_OS_MAC
	#include <mach/mach.h>
	#include <pthread.h>
	#include <semaphore.h>
#endif

#include <atomic>

namespace yu
{

#if defined YU_OS_WIN32
	typedef DWORD ThreadReturn;
	typedef PVOID ThreadContext;
	#define ThreadCall WINAPI
	typedef DWORD ThreadHandle;
#else
	typedef void* ThreadReturn;
	typedef void* ThreadContext;
	#define ThreadCall 
	typedef pthread_t ThreadHandle;
#endif

typedef ThreadReturn ThreadCall ThreadFunc(ThreadContext);

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
	ThreadHandle handle;
};

struct Mutex
{
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

struct RWMutex
{
	RWMutex();
	~RWMutex();

	void ReaderLock();

};

struct ScopedLock
{
	ScopedLock(Mutex& m);
	~ScopedLock();

	Mutex& m;
};

struct Locker
{
	Locker(Mutex& m);
	~Locker();

	void Lock();

	bool locked;
	Mutex& m;
};

struct CondVar
{
	CondVar();
	~CondVar();

#if defined YU_OS_WIN32
	CONDITION_VARIABLE cv;
#else
	pthread_cond_t cv;
#endif
};
void			WaitForCondVar(CondVar& cv, Mutex& m);
void			NotifyCondVar(CondVar& cv);
void			NotifyAllCondVar(CondVar& cv);

struct Event
{
	Event() : signaled(false)
	{}
	CondVar	cv;
	Mutex	cs;
	std::atomic<bool>	signaled;
};
void			WaitForEvent(Event& ev);
void			SignalEvent(Event& ev);
void			ResetEvent(Event& ev);

struct Semaphore
{
	Semaphore(int initCount, int maxCount);
	~Semaphore();

#if defined YU_OS_WIN32
	HANDLE sem;
#elif defined YU_OS_MAC
	sem_t* sem;
#else
	error semaphore not implemented
#endif
};
void			WaitForSem(Semaphore& sem);
void			SignalSem(Semaphore& sem);

void			InitThreadRuntime();
void			FreeThreadRuntime();
Thread			CreateThread(ThreadFunc func, ThreadContext context, ThreadPriority priority = NormalPriority, u64 affinityMask = 0);
ThreadHandle	GetCurrentThreadHandle();
u64				GetThreadAffinity(ThreadHandle thread);
void			SetThreadAffinity(ThreadHandle threadHandle, u64 affinityMask);
void			SetThreadName(ThreadHandle thread, const char* name);
void			SetThreadPriority(ThreadHandle thread, ThreadPriority priority);
bool			AllThreadExited();
unsigned int	NumThreads();

struct			FrameLock;
FrameLock*		AddFrameLock();
void			WaitForKick(FrameLock* lock);
void			FrameComplete(FrameLock* lock);

}

#endif
