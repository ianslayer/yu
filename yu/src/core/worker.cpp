#include "../container/array.h"
#include "../container/dequeue.h"
#include "thread.h"
#include "timer.h"
#include "worker.h"
#include "system.h"
#include "log.h"
#include <atomic>
namespace yu
{
#define MAX_WORKER_THREAD 32
#define MAX_WORK_QUEUE_LEN 4096

struct WorkItem
{
	WorkFunc*				func;
	void*					constData;
	void*					mutableData;

	std::atomic<int>		permits;

	Array<WorkItem*>		depdentList;
	Array<WorkItem*>		permitList;
};

void TerminateWorkFunc(const void* constData, void* mutableData, const WorkItem* dep, int numDep) {}
void FrameEndWorkFunc(const void* constData, void* mutableData, const WorkItem* dep, int numDep)
{
	FrameLock* lock = (FrameLock*)mutableData;
	WaitForKick(lock);
	DummyWorkLoad(10);
	FrameComplete(lock);
}


bool YuRunning();
ThreadReturn ThreadCall WorkerThreadFunc(ThreadContext context);
struct WorkerSystem
{
	WorkerSystem(unsigned int coreCount) :workQueueSem(0, MAX_WORK_QUEUE_LEN)
	{
		numWorkerThread = coreCount - 2;
	
		terminateWorkItem.func = TerminateWorkFunc;
		frameLock = AddFrameLock();
		frameEndWorkItem.mutableData = frameLock;
		frameEndWorkItem.func = FrameEndWorkFunc;
	}

	Thread					workerThread[MAX_WORKER_THREAD];
	unsigned int			numWorkerThread = 0;
	ArenaAllocator			workerArena;

	MpmcFifo<WorkItem*, MAX_WORK_QUEUE_LEN>	workQueue;
	Semaphore				workQueueSem;

	WorkItem				terminateWorkItem;

	FrameLock*				frameLock;
	WorkItem				frameStartWorkItem;
	WorkItem				frameEndWorkItem;
};
static WorkerSystem* gWorkerSystem;
ThreadReturn ThreadCall WorkerThreadFunc(ThreadContext context)
{
	Log("Worker thread started\n");
	while (YuRunning())
	{
		WaitForSem(gWorkerSystem->workQueueSem); //TODO: eliminate gWorkerSystem here, should be passed in as parameter
		WorkItem* item;
		while (gWorkerSystem->workQueue.Dequeue(item))
		{
			item->func(item->constData, item->mutableData, nullptr, 0);
		}
	}
	Log("Worker thread exit\n");
	return 0;
}

void SubmitWorkItem(WorkItem* item, WorkItem* dep, int numDep)
{
	gWorkerSystem->workQueue.Enqueue(item);
	SignalSem(gWorkerSystem->workQueueSem);
}

void SubmitTerminateWork()
{
	for (int i = 0; i < gWorkerSystem->numWorkerThread; i++)
	{
		SubmitWorkItem(&gWorkerSystem->terminateWorkItem, nullptr, 0);
	}
}

WorkItem* FrameStartWorkItem()
{
	return &gWorkerSystem->frameEndWorkItem;
}

void InitWorkerSystem()
{
	CPUInfo cpuInfo = System::GetCPUInfo();
	gWorkerSystem = new WorkerSystem(cpuInfo.numLogicalProcessors);
	
	for (int i = 0; i < gWorkerSystem->numWorkerThread; i++)
	{
		gWorkerSystem->workerThread[i] = CreateThread(WorkerThreadFunc, nullptr, NormalPriority, 1 << (i+1));
	}
}

void FreeWorkerSystem()
{
	delete gWorkerSystem;
}

}