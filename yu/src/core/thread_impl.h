#include "thread.h"

namespace yu
{
#define MAX_THREAD 32


YU_PRE_ALIGN(CACHE_LINE)
struct FrameLock
{
	FrameLock() : frameCount(0){}

	Event				event;
	std::atomic<u64>	frameCount;

} YU_POST_ALIGN(CACHE_LINE);


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

YU_GLOBAL ThreadTable* gThreadTable;
YU_GLOBAL Event* gFrameSync;

void InitThreadRuntime(Allocator* allocator)
{
	gThreadTable = NewAligned<ThreadTable>(allocator, CACHE_LINE);
	gFrameSync = New<Event>(allocator);
}

void FreeThreadRuntime(Allocator* allocator)
{
	Delete(allocator, gFrameSync);
	DeleteAligned(allocator, gThreadTable);
}

void FakeKickStart() //for clear thread;
{
	//gThreadTable->frameCount.fetch_add(1);
	SignalEvent(*gFrameSync);
	gThreadTable->frameCount.fetch_add(1);
}

void KickStart()
{
	SignalEvent(*gFrameSync); //TODO: eliminate gFrameSync... shared state
	/*
	u64 globalFrameCount = gThreadTable->frameCount.load(std::memory_order_relaxed);
	
	for (unsigned int i = 0; i < gThreadTable->numLocks; i++) //wait for all thread kicked
	{
		u64 frameCount = gThreadTable->frameLockList[i].frameCount.load(std::memory_order_acquire);
		while (frameCount <= globalFrameCount)
			frameCount = gThreadTable->frameLockList[i].frameCount.load(std::memory_order_acquire);
	}
	*/
	gThreadTable->frameCount.fetch_add(1, std::memory_order_release);
	//ResetEvent(*gFrameSync);
}

void WaitForKick(FrameLock* lock)
{
	WaitForEvent(*gFrameSync);
	lock->frameCount.fetch_add(1, std::memory_order_release);

	u64 globalFrameCount = gThreadTable->frameCount.load(std::memory_order_acquire);
	u64 frameCount = lock->frameCount.load(std::memory_order_relaxed);
	while (frameCount > globalFrameCount)
	{//spin a little if we are ahead of main thread
		//TODO: should replaced with a barrier
		globalFrameCount = gThreadTable->frameCount.load(std::memory_order_acquire);
	}

}

void WaitFrameComplete()
{
	for (unsigned int i = 0; i < gThreadTable->numLocks; i++) //TODO: change this to barrier
	{
		WaitForEvent(gThreadTable->frameLockList[i].event);
		ResetEvent(gThreadTable->frameLockList[i].event);
	}
	ResetEvent(*gFrameSync);
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

bool AllThreadsExited()
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
