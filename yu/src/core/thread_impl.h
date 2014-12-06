#include "thread.h"
#include "timer.h"
#include "allocator.h"
#include <atomic>
#include <new>

namespace yu
{
#define MAX_THREAD 32
#define CACHE_LINE 64

ScopedLock::ScopedLock(Mutex& _m) : m(_m)
{
	m.Lock();
}

ScopedLock::~ScopedLock()
{
	m.Unlock();
}

Locker::Locker(Mutex& _m) : locked(false), m(_m)
{
}

Locker::~Locker()
{
	if (locked)
		m.Unlock();
}

void Locker::Lock()
{
	m.Lock();
	locked = true;
}

YU_PRE_ALIGN(CACHE_LINE)
struct FrameLock
{
	FrameLock() : frameCount(0){}

	Event				event;
	std::atomic<u64>	frameCount;

} YU_POST_ALIGN(CACHE_LINE);


void WaitForEvent(Event& ev)
{

	if (ev.signaled == true)
	{
		return;
	}

	ScopedLock lock(ev.cs);
	if (ev.signaled == true)
	{
		return;
	}
	WaitForCondVar(ev.cv, ev.cs);

}

void SignalEvent(Event& ev)
{
	ScopedLock lock(ev.cs);
	ev.signaled = true;
	NotifyAllCondVar(ev.cv);
}

void ResetEvent(Event& ev)
{
	ScopedLock lock(ev.cs);
	ev.signaled = false;
}

struct ThreadTable
{
	ThreadTable() : numThreads(0), numLocks(0), frameCount(0){}
	enum ThreadState
	{
		THREAD_RUNNING = 0,
		THREAD_EXIT,
	};

	YU_PRE_ALIGN(CACHE_LINE)
	struct ThreadEntry
	{
		ThreadEntry() : threadState(THREAD_RUNNING){}
		std::atomic<int>	threadState;
		ThreadHandle		threadHandle;
	} YU_POST_ALIGN(CACHE_LINE);

	FrameLock					frameLockList[MAX_THREAD];
	ThreadEntry					threadList[MAX_THREAD];
	std::atomic<unsigned int>	numThreads ;
	std::atomic<unsigned int>	numLocks ;
	std::atomic<u64>			frameCount;
};

static ThreadTable* gThreadTable;
static Event* gFrameSync;

void InitThreadRuntime()
{
	gThreadTable = new(gDefaultAllocator->AllocAligned(sizeof(ThreadTable), CACHE_LINE)) ThreadTable();
	gFrameSync = new Event;
}

void FreeThreadRuntime()
{
	gThreadTable->~ThreadTable();
	gDefaultAllocator->FreeAligned(gThreadTable);
}

void FakeKickStart() //for clear thread;
{
	//gThreadTable->frameCount.fetch_add(1);
	SignalEvent(*gFrameSync);
}

void KickStart()
{
	SignalEvent(*gFrameSync); //TODO: eliminate gFrameSync... shared state

	u64 globalFrameCount = gThreadTable->frameCount.load(std::memory_order_relaxed);
	
	for (unsigned int i = 0; i < gThreadTable->numLocks; i++) //wait for all thread kicked
	{
		u64 frameCount = gThreadTable->frameLockList[i].frameCount.load(std::memory_order_acquire);
		while (frameCount <= globalFrameCount)
			frameCount = gThreadTable->frameLockList[i].frameCount.load(std::memory_order_acquire);
	}
	ResetEvent(*gFrameSync);
}

void WaitForKick(FrameLock* lock)
{
	WaitForEvent(*gFrameSync);
	u64 globalFrameCount = gThreadTable->frameCount.load(std::memory_order_acquire);
	u64 frameCount = lock->frameCount.load(std::memory_order_relaxed);
	while (frameCount > globalFrameCount)
	{//spin a little if we are ahead of main thread
		//TODO: should replaced with a barrier
		globalFrameCount = gThreadTable->frameCount.load(std::memory_order_acquire);
	}

	lock->frameCount.fetch_add(1, std::memory_order_release);
}

void WaitFrameComplete()
{
	for (unsigned int i = 0; i < gThreadTable->numLocks; i++) //TODO: change this to barrier
	{
		WaitForEvent(gThreadTable->frameLockList[i].event);
		ResetEvent(gThreadTable->frameLockList[i].event);
	}
	//ResetEvent(*gFrameSync);
	gThreadTable->frameCount.fetch_add(1, std::memory_order_release);

}

void FrameComplete(FrameLock* lock)
{
	SignalEvent(lock->event);
}

void RegisterThread(ThreadHandle handle)
{
	unsigned int slot = gThreadTable->numThreads.fetch_add(1);
	gThreadTable->threadList[slot].threadHandle = handle;
}

FrameLock*	AddFrameLock()
{
	unsigned int slot = gThreadTable->numLocks.fetch_add(1);
	return &gThreadTable->frameLockList[slot];
}

void ThreadExit(ThreadHandle handle)
{
	for (unsigned int i = 0; i < gThreadTable->numThreads; i++)
	{
		if (gThreadTable->threadList[i].threadHandle == handle)
		{
			gThreadTable->threadList[i].threadState.exchange(ThreadTable::THREAD_EXIT);
			break;
		}
	}
}

unsigned int NumThreads()
{
	return gThreadTable->numThreads;
}

bool AllThreadExited()
{
	for (unsigned int i = 0; i < gThreadTable->numThreads; i++)
	{
		if (gThreadTable->threadList[i].threadState == ThreadTable::THREAD_RUNNING)
		{
			return false;
		}
	}
	
	return true;
}


}