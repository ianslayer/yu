#include "thread.h"
#include "timer.h"
#include <atomic>
namespace yu
{
#define MAX_THREAD 32

struct FrameLock
{
	FrameLock() : frameCount(0){}
	Event				event;
	u64					frameCount;
	ThreadHandle		threadHandle;
	int					threadIndex = -1;
};

void WaitForEvent(Event& ev, bool autoReset)
{
	ScopedLock lock(ev.cs);
	if (ev.signaled == true)
	{
		if(autoReset)
			ev.signaled = false;
		return;
	}
	WaitForCondVar(ev.cv, ev.cs);
	if(autoReset)
		ev.signaled = false;
}

void SignalEvent(Event& ev)
{
	ev.cs.Lock();
	ev.signaled = true;
	NotifyAllCondVar(ev.cv);
	ev.cs.Unlock();
}

void ResetEvent(Event& ev)
{
	ev.cs.Lock();
	ev.signaled = false;
	ev.cs.Unlock();
}

struct ThreadTable
{
	ThreadTable() : numThreads(0), numLocks(0), frameCount(0){}
	enum ThreadState
	{
		THREAD_RUNNING = 0,
		THREAD_EXIT,
	};

	struct ThreadEntry
	{
		ThreadEntry() : threadState(THREAD_RUNNING){}
		std::atomic<int>	threadState;
		ThreadHandle		threadHandle;
	};

	ThreadEntry					threadList[MAX_THREAD];
	FrameLock					frameLockList[MAX_THREAD]; // TODO: this is prone to false sharing
	std::atomic<unsigned int>	numThreads ;
	std::atomic<unsigned int>	numLocks ;
	std::atomic<u64>			frameCount;
};

static ThreadTable* gThreadTable;
static Event* gFrameSync;

void InitThreadRuntime()
{
	gThreadTable = new ThreadTable();
	gFrameSync = new Event;
	std::atomic_thread_fence(std::memory_order_seq_cst);
}

void FreeThreadRuntime()
{
	delete gThreadTable;
}

void FakeKickStart() //for clear thread;
{
	//gThreadTable->frameCount.fetch_add(1);
	SignalEvent(*gFrameSync);
}

void KickStart()
{
	SignalEvent(*gFrameSync);
}

void WaitForKick(FrameLock* lock)
{
	//std::atomic_thread_fence(std::memory_order_acquire);
	WaitForEvent(*gFrameSync);
	u64 globalFrameCount = gThreadTable->frameCount.load(std::memory_order_acquire);
	while (lock->frameCount > globalFrameCount)
	{//spin a little if we are ahead of main thread
		//TODO: should replaced with a barrier
		globalFrameCount = gThreadTable->frameCount.load(std::memory_order_acquire);
	}
}

void WaitFrameComplete()
{
	for (unsigned int i = 0; i < gThreadTable->numLocks; i++)
	{
		WaitForEvent(gThreadTable->frameLockList[i].event);
		ResetEvent(gThreadTable->frameLockList[i].event);
	}
	ResetEvent(*gFrameSync);
	gThreadTable->frameCount.fetch_add(1, std::memory_order_release);

}

void FrameComplete(FrameLock* lock)
{
	lock->frameCount++;
	SignalEvent(lock->event);
}

void RegisterThread(ThreadHandle handle)
{
	unsigned int slot = gThreadTable->numThreads.fetch_add(1);
	gThreadTable->threadList[slot].threadHandle = handle;
}

FrameLock*	AddFrameLock(ThreadHandle handle)
{
	unsigned int slot = gThreadTable->numLocks.fetch_add(1);
	gThreadTable->frameLockList[slot].threadHandle = handle;
	for (unsigned int i = 0; i < gThreadTable->numThreads; i++)
	{
		if (gThreadTable->threadList[i].threadHandle == handle)
		{
			gThreadTable->frameLockList[slot].threadIndex = (int)i;
			break;
		}
	}

	return &gThreadTable->frameLockList[slot];
}

void ThreadExit(ThreadHandle handle)
{
	for (unsigned int i = 0; i < gThreadTable->numThreads; i++)
	{
		if (gThreadTable->threadList[i].threadHandle == handle)
		{
			gThreadTable->threadList[i].threadState.exchange(ThreadTable::THREAD_EXIT, std::memory_order_seq_cst);
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

void DummyWorkLoad(double timeInMs)
{
	PerfTimer startTime;
	startTime.Start();
	double deltaT = 0;
	while (1)
	{
		if (deltaT >= timeInMs)
			break;
		PerfTimer endTime;
		endTime.Start();

		CycleCount count;
		count.cycle = endTime.cycleCounter.cycle - startTime.cycleCounter.cycle;
		deltaT = ConvertToMs(count);
	}
}


}