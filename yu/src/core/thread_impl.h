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



struct ThreadContextStack
{
	ThreadContextStackEntry entry[128];
	int top;
};

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
	ThreadContextStack	stack;
} YU_POST_ALIGN(CACHE_LINE);
YU_THREAD_LOCAL unsigned int threadIndex;
	
struct ThreadTable
{
	ThreadTable() : numThreads(), numLocks(0), frameCount(0){}


	FrameLock					frameLockList[MAX_THREAD];
	ThreadEntry					threadList[MAX_THREAD];
	std::atomic<unsigned int>	numThreads ;
	std::atomic<unsigned int>	numLocks ;
	std::atomic<u64>			frameCount;

	Allocator*					defaultAllocator;
};
	
YU_GLOBAL ThreadTable* gThreadTable;
YU_GLOBAL Event* gFrameSync;
	
ThreadEntry* GetThreadEntry()
{
	return gThreadTable->threadList + threadIndex;		
}

ThreadContextStack* GetThreadContextStack()
{
	return &GetThreadEntry()->stack;	
}

const ThreadContextStackEntry* GetCurrentThreadStackEntry()
{
	ThreadContextStack* stack = GetThreadContextStack();
	assert(stack->top >= 1);
	return &stack->entry[stack->top-1];
}
	
void PushThreadContext(const ThreadContextStackEntry& entry)
{
	ThreadContextStack* stack = GetThreadContextStack();
	assert(stack->top < ARRAY_SIZE(stack->entry));
	if(stack->top >= ARRAY_SIZE(stack->entry))
	{
		Log("error, PushThreadContext: thread stack full\n");
		return ;
	}

	stack->entry[stack->top] = entry;
	stack->top++;
}

ThreadContextStackEntry PopThreadContext()
{
	ThreadContextStack* stack = GetThreadContextStack();	
	assert(stack->top >= 2);
	ThreadContextStackEntry entry {};
	if(stack->top == 1)
	{
		Log("error, PopThreadContext: try to pop default context\n");
		return entry;
	}
	stack->top--;
	entry = stack->entry[stack->top];
	stack->entry[stack->top] = {};
	return entry;
}

void InitThreadContextStack( Allocator* defaultAllocator)
{
	ThreadContextStack* stack = GetThreadContextStack();
	memset(stack, 0, sizeof(*stack));
	
	ThreadContextStackEntry defaultEntry;
	defaultEntry.allocator = defaultAllocator;
	PushThreadContext(defaultEntry);
}
	
void RegisterThread(ThreadHandle thread);
void InitThreadRuntime(Allocator* defaultAllocator)
{	
	gThreadTable = NewAligned<ThreadTable>(defaultAllocator, CACHE_LINE);
	gFrameSync = New<Event>(defaultAllocator);

	gThreadTable->defaultAllocator = defaultAllocator;
	//register main thread;
	ThreadHandle mainThreadHandle = GetCurrentThreadHandle();
	RegisterThread(mainThreadHandle);	
}

void FreeThreadRuntime(Allocator* allocator)
{
	Delete(allocator, gFrameSync);
	DeleteAligned(allocator, gThreadTable);
}

void FinalKickStart() //for clear thread;
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

u64 FrameCount()
{
	return gThreadTable->frameCount;	
}

void RegisterThread(ThreadHandle handle)
{
	unsigned int slot = gThreadTable->numThreads.fetch_add(1);
	threadIndex = slot;
	gThreadTable->threadList[slot].threadHandle = handle;

	InitThreadContextStack(gThreadTable->defaultAllocator);
}

FrameLock*	AddFrameLock()
{
	unsigned int slot = gThreadTable->numLocks.fetch_add(1);
	return &gThreadTable->frameLockList[slot];
}

void ThreadExit(ThreadHandle handle)
{
	for (unsigned int i = 1; i < gThreadTable->numThreads; i++)
	{
		if (gThreadTable->threadList[i].threadHandle == handle)
		{
			gThreadTable->threadList[i].threadState.exchange(THREAD_EXIT);
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
	for (unsigned int i = 1; i < gThreadTable->numThreads; i++) // thread 0 is main thread
	{
		if (gThreadTable->threadList[i].threadState == THREAD_RUNNING)
		{
			return false;
		}
	}
	return true;
}


}
