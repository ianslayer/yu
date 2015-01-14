#include "worker.h"

namespace yu
{

#define MAX_WORKER_THREAD 32
#define MAX_WORK_QUEUE_LEN 4096

struct WorkLink
{
	WorkLink(Allocator* allocator) : dependList(4, allocator), permitList(16, allocator)
	{	}
	Array<WorkItem*>		dependList;
	Array<WorkItem*>		permitList;
};

YU_PRE_ALIGN(CACHE_LINE)
struct WorkItem
{
	WorkItem(Allocator* allocator = gDefaultAllocator) : dependList(4, allocator), permitList(16, allocator), 
		func(nullptr), finalizer(nullptr), outputData(nullptr), inputData(nullptr)
	{
		permits = 0;
		isDone = false;
#if defined YU_DEBUG
		dbgFrameCount = 0;
#endif
	}

	~WorkItem()
	{
		//TODO: where should we release the data ?
		//Allocator* allocator = dependList.GetAllocator();
	}

	Array<WorkItem*>		dependList;
	Array<WorkItem*>		permitList;

	//WorkLink*				link;

	Mutex					lock;

	WorkFunc*				func;
	Finalizer*				finalizer;

	InputData*				inputData;
	OutputData*				outputData;

	std::atomic<int>		permits;
	std::atomic<bool>		isDone;

#if defined YU_DEBUG
	std::atomic<u64>		dbgFrameCount;
#endif
} YU_POST_ALIGN(CACHE_LINE);


YU_PRE_ALIGN(CACHE_LINE)
struct WorkerThread
{
	int						id = -1;
	Thread					thread;
	ArenaAllocator			workerFrameArena;
	Array<WorkItem*>		retiredItemPermitList; //TODO: move this into retired function, this should use a stack allocator (alloca)
} YU_POST_ALIGN(CACHE_LINE);


void FreeSysWorkItem(WorkItem* item)
{
	if (item->finalizer)
	{
		item->finalizer(item);
	}
	/*
	if (item->inputData)
	{
		FreeInputData(item);
	}

	if (item->outputData)
	{
		FreeOutputData(item);
	}
	*/
	item->~WorkItem();
}

WorkItem* NewSysWorkItem()
{
	return new(gSysArena->AllocAligned(sizeof(WorkItem), CACHE_LINE)) WorkItem(gSysArena);
}

/*
InputData* AllocInputData(WorkItem* item, size_t size)
{
	InputData* data = (InputData*) gDefaultAllocator->Alloc(size);
	data->source = gDefaultAllocator;
	item->inputData = data;
	return data;
}

void FreeInputData(WorkItem* item)
{
	Allocator* allocator = item->inputData->source;
	allocator->Free(item->inputData);
	item->inputData = nullptr;
}
*/

InputData* GetInputData(WorkItem* item)
{
	return item->inputData;
}

void SetInputData(WorkItem* item, InputData* input)
{
	item->inputData = input;
}

/*
OutputData* AllocOutputData(WorkItem* item, size_t size)
{
	OutputData* data = (OutputData*)gDefaultAllocator->Alloc(size);
	data->source = gDefaultAllocator;
	item->outputData = data;
	return data;
}

void FreeOutputData(WorkItem* item)
{
	Allocator* allocator = item->outputData->source;
	allocator->Free(item->outputData);
	item->outputData = nullptr;
}
*/

OutputData* GetOutputData(WorkItem* item)
{
	//TODO: check dependcy when access?
	return item->outputData;
}

void SetOutputData(WorkItem* item, OutputData* output)
{
	item->outputData = output;
}

int	GetNumDepend(WorkItem* item)
{
	return item->dependList.Size();
}

OutputData* GetDependOutputData(WorkItem* item, int i)
{
	assert(i < item->dependList.Size());
	return GetOutputData(item->dependList[i]);
}

void SetWorkFunc(WorkItem* item, WorkFunc* func, Finalizer* finalizer)
{
	item->func = func;
	item->finalizer = finalizer;
}

void TerminateWorkFunc(WorkItem* item) {}

bool YuRunning();
ThreadReturn ThreadCall WorkerThreadFunc(ThreadContext context);
struct WorkerSystem
{
	WorkerSystem() :workQueueSem(0, MAX_WORK_QUEUE_LEN)
	{
		CPUInfo cpuInfo = SystemInfo::GetCPUInfo();
		numWorkerThread = cpuInfo.numLogicalProcessors - 2;
	
		SetWorkFunc(&terminateWorkItem, TerminateWorkFunc, nullptr);
	}

	WorkerThread			workerThread[MAX_WORKER_THREAD]; //0 is main thread
	unsigned int			numWorkerThread = 0;		

	MpmcFifo<WorkItem*, MAX_WORK_QUEUE_LEN>	workQueue;
	Semaphore				workQueueSem;

	WorkItem				terminateWorkItem;

};

YU_GLOBAL WorkerSystem* gWorkerSystem;
YU_THREAD_LOCAL WorkerThread* worker;

WorkerThread* GetWorkerThread()
{
	return worker;
}

int GetWorkerThreadIdx()
{
	return worker->id;
}

void SubmitWorkItem(WorkItem* item, WorkItem* dep[], int numDep)
{
	WorkerThread* worker = GetWorkerThread();
	item->permits = -numDep;
	for (int i = 0; i < numDep; i++)
	{
		item->dependList.PushBack(dep[i]);
	}
	if (numDep == 0)
	{
		gWorkerSystem->workQueue.Enqueue(item);
		SignalSem(gWorkerSystem->workQueueSem);
		return;
	}
	for (int i = 0; i < numDep; i++)
	{
		ScopedLock lock(dep[i]->lock);
		
		if (!dep[i]->isDone.load(std::memory_order_acquire))
		{
			dep[i]->permitList.PushBack(item); 
		}
		else
		{
			int prev = item->permits.fetch_add(1);
			if (prev == -1)
			{
				gWorkerSystem->workQueue.Enqueue(item);
				SignalSem(gWorkerSystem->workQueueSem);
			}
		}

	}
}

void Complete(WorkItem* item, Array<WorkItem*>& permitList)
{
	item->lock.Lock();
	item->isDone.store(true, std::memory_order_release);
	permitList = item->permitList;
	item->lock.Unlock();

	for (int i = 0; i < permitList.Size(); i++)
	{
		WorkItem* p = permitList[i];
		int prev = p->permits.fetch_add(1);

		if (prev == -1)
		{
			gWorkerSystem->workQueue.Enqueue(p);
			SignalSem(gWorkerSystem->workQueueSem);
		}
	}	
#if defined YU_DEBUG
	item->dbgFrameCount++;
#endif
}

ThreadReturn ThreadCall WorkerThreadFunc(ThreadContext context)
{
	worker = (WorkerThread*)context;
	Log("Worker thread:%d started\n", worker->id);

	while (YuRunning())
	{
		WaitForSem(gWorkerSystem->workQueueSem); //TODO: eliminate gWorkerSystem here, should be passed in as parameter
		WorkItem* item;
		while (gWorkerSystem->workQueue.Dequeue(item))
		{
			item->func(item);
			worker->retiredItemPermitList.Clear();
			Complete(item, worker->retiredItemPermitList);
		}
	}
	Log("Worker thread:%d exit\n", worker->id);
	return 0;
}

bool WorkerFrameComplete();
void GetMainThreadWorker()
{
	worker = &gWorkerSystem->workerThread[0];
}
void MainThreadWorker()
{
	while (!WorkerFrameComplete())
	{
		//WaitForSem(gWorkerSystem->workQueueSem); //this is asymetric..., but if main thread wait here it could hang forever
		WorkItem* item;
		while (gWorkerSystem->workQueue.Dequeue(item))
		{
			item->func(item);
			worker->retiredItemPermitList.Clear();
			Complete(item, worker->retiredItemPermitList);
		}
	}
}

void ResetWorkItem(WorkItem* item)
{
	item->permitList.Clear();
	item->dependList.Clear();
	item->permits = 0;
	item->isDone.store(false);
}

bool IsDone(WorkItem* item)
{
	return item->isDone.load(std::memory_order_acquire);
}


void SubmitTerminateWork()
{
	for (unsigned int i = 0; i < gWorkerSystem->numWorkerThread; i++)
	{
		SubmitWorkItem(&gWorkerSystem->terminateWorkItem, nullptr, 0);
	}
}

void InitWorkerSystem()
{

	gWorkerSystem = NewAligned<WorkerSystem>(gSysArena, CACHE_LINE);

	//main thread
	gWorkerSystem->workerThread[0].id = 0;
	gWorkerSystem->workerThread[0].thread.handle = GetCurrentThreadHandle();
	SetThreadAffinity(gWorkerSystem->workerThread[0].thread.handle, 1);
	
	for (unsigned int i = 1; i < gWorkerSystem->numWorkerThread + 1; i++) //zero is main thread
	{
		gWorkerSystem->workerThread[i].id = (int)i;
		gWorkerSystem->workerThread[i].thread = CreateThread(WorkerThreadFunc, &gWorkerSystem->workerThread[i], NormalPriority, 1 << (i));
	}

	GetMainThreadWorker(); //init main thread worker tls
}

void FreeWorkerSystem()
{
	DeleteAligned(gSysArena, gWorkerSystem);
}

}