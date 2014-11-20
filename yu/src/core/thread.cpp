#include "thread.h"
#include "timer.h"
#include <atomic>
namespace yu
{
#define MAX_THREAD 32

struct FrameLock
{
	Event				event;
	std::atomic<u64>	frameCount = 0;	
	ThreadHandle		threadHandle;
	int					threadIndex = -1;
	bool				frameStarted = false;
	Mutex				frameStartedCs;
};

void WaitForEvent(Event& ev)
{
	ev.cs.Lock();
	if (ev.signaled == true)
	{
		ev.cs.Unlock();
		return;
	}
	WaitForCondVar(ev.cv, ev.cs);
	ev.cs.Unlock();
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
	enum ThreadState
	{
		THREAD_RUNNING = 0,
		THREAD_EXIT,
	};

	struct ThreadEntry
	{
		std::atomic<int>	threadState = THREAD_RUNNING;
		ThreadHandle		threadHandle;
	};

	ThreadEntry					threadList[MAX_THREAD];
	FrameLock					frameLockList[MAX_THREAD];
	std::atomic<unsigned int>	numThreads = 0;
	std::atomic<unsigned int>	numLocks = 0;
	std::atomic<u64>			frameCount = 0;
};

static ThreadTable* gThreadTable = nullptr;

void InitThreadRuntime()
{
	gThreadTable = new ThreadTable();
	std::atomic_thread_fence(std::memory_order_seq_cst);
}

void FreeThreadRuntime()
{
	delete gThreadTable;
}


extern Event* gFrameSync;


#ifdef BUZY_WAIT
void FakeKickStart() //for clear thread;
{
	//gThreadTable->frameCount.fetch_add(1);
	SignalEvent(*gFrameSync);
}

void KickStart()
{

	for (unsigned int i = 0; i < gThreadTable->numLocks; i++)
	{
		for (;;)
		{
			gThreadTable->frameLockList[i].frameStartedCs.Lock();
			if (gThreadTable->frameLockList[i].frameStarted)
			{
				gThreadTable->frameLockList[i].frameStartedCs.Unlock();
				break;
			}
			gThreadTable->frameLockList[i].frameStartedCs.Unlock();
		}
	}
}

void WaitForKick(FrameLock* lock)
{

	while (lock->frameCount > gThreadTable->frameCount)
	{//spin a little if we are ahead of main thread
		//	std::atomic_thread_fence(std::memory_order_acquire);
	}
	lock->frameStartedCs.Lock();
	lock->frameStarted = true;
	lock->frameStartedCs.Unlock();
}

void WaitFrameComplete()
{
	
	for (unsigned int i = 0; i < gThreadTable->numLocks; i++)
	{
		while (1)
		{
			gThreadTable->frameLockList[i].event.cs.Lock();
			if (gThreadTable->frameLockList[i].event.signaled == true)
			{
				gThreadTable->frameLockList[i].event.cs.Unlock();
				ResetEvent(gThreadTable->frameLockList[i].event);
				gThreadTable->frameLockList[i].frameStartedCs.Lock();
				gThreadTable->frameLockList[i].frameStarted = false;
				gThreadTable->frameLockList[i].frameStartedCs.Unlock();
				break;
			}
			gThreadTable->frameLockList[i].event.cs.Unlock();
		}

		//WaitForEvent(gThreadTable->frameLockList[i].event);
	}

	gThreadTable->frameCount.fetch_add(1);

}

void FrameComplete(FrameLock* lock)
{
	lock->frameCount.fetch_add(1);
	//SignalEvent(lock->event);
}

#else

void FakeKickStart() //for clear thread;
{
	//gThreadTable->frameCount.fetch_add(1);
	SignalEvent(*gFrameSync);
}

void KickStart()
{
	//gThreadTable->frameCount.fetch_add(1);
	SignalEvent(*gFrameSync);

	for (unsigned int i = 0; i < gThreadTable->numLocks; i++)
	{
		for (;;)
		{
			gThreadTable->frameLockList[i].frameStartedCs.Lock();
			if (gThreadTable->frameLockList[i].frameStarted)
			{
				gThreadTable->frameLockList[i].frameStartedCs.Unlock();
				break;
			}
			gThreadTable->frameLockList[i].frameStartedCs.Unlock();
		}
	}
	ResetEvent(*gFrameSync);
}

void WaitForKick(FrameLock* lock)
{
	//std::atomic_thread_fence(std::memory_order_acquire);
	WaitForEvent(*gFrameSync);
	while (lock->frameCount > gThreadTable->frameCount)
	{//spin a little if we are ahead of main thread
	//	std::atomic_thread_fence(std::memory_order_acquire);
	}	
	lock->frameStartedCs.Lock();
	lock->frameStarted = true;
	lock->frameStartedCs.Unlock();
}

void WaitFrameComplete()
{
	
	for (unsigned int i = 0; i < gThreadTable->numLocks; i++)
	{
		WaitForEvent(gThreadTable->frameLockList[i].event);
		ResetEvent(gThreadTable->frameLockList[i].event);
		gThreadTable->frameLockList[i].frameStartedCs.Lock();
		gThreadTable->frameLockList[i].frameStarted = false;
		gThreadTable->frameLockList[i].frameStartedCs.Unlock();
	}
//	ResetEvent(*gFrameSync); //TODO, better move this as early as possible, but must sure all thread is started
	gThreadTable->frameCount.fetch_add(1);

}

void FrameComplete(FrameLock* lock)
{
	lock->frameCount.fetch_add(1);
	SignalEvent(lock->event);
}

#endif

void RegisterThread(ThreadHandle handle)
{
	unsigned int slot = gThreadTable->numThreads.fetch_add(1, std::memory_order_seq_cst);
	gThreadTable->threadList[slot].threadHandle = handle;
}

FrameLock*	AddFrameLock(ThreadHandle handle)
{
	unsigned int slot = gThreadTable->numLocks.fetch_add(1, std::memory_order_seq_cst);
	gThreadTable->frameLockList[slot].threadHandle = handle;
	for (unsigned int i = 0; i < gThreadTable->numThreads; i++)
	{
		if (gThreadTable->threadList[i].threadHandle == handle)
		{
			gThreadTable->frameLockList[slot].threadIndex = i;
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