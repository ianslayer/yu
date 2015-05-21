
#include "worker.h"
#include "free_list.h"
#include "../math/yu_math.h"
#include "../renderer/renderer.h"
#include "../yu.h"

namespace yu
{

#define MAX_WORK_QUEUE_LEN 1024
#define MAX_WORK_ITEM 4096

RenderQueue* CreateRenderQueue(Renderer* renderer);
	
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
	WorkItem(Allocator* allocator) :  permitList(16, allocator), 
		func(nullptr), finalizer(nullptr), outputData(nullptr), inputData(nullptr)
	{
		permits = 0;
		isDone = false;

		DEBUG_ONLY(dbgFrameCount = 0);
	}

	~WorkItem()
	{
		//TODO: where should we release the data ?
		//Allocator* allocator = dependList.GetAllocator();
	}
	Array<WorkItem*>		permitList;

	//WorkLink*				link;

	Mutex					lock;

	WorkFunc*				func;
	Finalizer*				finalizer;

	const InputData*		inputData;
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
	WorkerThread(Allocator* allocator): retiredItemPermitList(allocator) {}
	int						id = -1;
	Thread					thread;
	//ArenaAllocator			workerFrameArena;
	Array<WorkItem*>		retiredItemPermitList; //TODO: move this into retired function, this should use a stack allocator (alloca)
	struct RenderQueue*		renderQueue;
} YU_POST_ALIGN(CACHE_LINE);

void TerminateWorkFunc(WorkItem* item) {}
struct WorkerSystem
{
	WorkerSystem(Allocator* allocator) :workQueueSem(0, MAX_WORK_QUEUE_LEN), terminateWorkItem(allocator)
	{
		CPUInfo cpuInfo = SystemInfo::GetCPUInfo();
		numWorkerThread = yu::max((i32)cpuInfo.numLogicalProcessors - (i32)2, (i32)0);

		workerThread = DeepNewArray<WorkerThread>(allocator, numWorkerThread + 1);
		SetWorkFunc(&terminateWorkItem, TerminateWorkFunc, nullptr);

		workItemList = DeepNewArray<WorkItem>(allocator, MAX_WORK_ITEM);
	}

	WorkerThread*			workerThread; //0 is main thread
	unsigned int			numWorkerThread = 0;		

	MpmcFifo<WorkItem*, MAX_WORK_QUEUE_LEN>	workQueue;
	Semaphore				workQueueSem;

	WorkItem				terminateWorkItem;

	IndexFreeList<MAX_WORK_ITEM> workItemIdList;
	WorkItem*				workItemList;
};

YU_GLOBAL WorkerSystem* gWorkerSystem;

WorkItemHandle NewWorkItem()
{
	WorkItemHandle item;
	item.id = gWorkerSystem->workItemIdList.Alloc();
	return item;
}

void FreeWorkItem(WorkItemHandle item)
{
	gWorkerSystem->workItemIdList.DeferredFree(item.id);
}

WorkItem* GetWorkItem(WorkItemHandle itemHandle)
{
	return &gWorkerSystem->workItemList[itemHandle.id];
}

WorkData GetWorkData(WorkItem* item)
{
	WorkData data;
	data.inputData = item->inputData;
	data.outputData = item->outputData;
	return data;
}

void SetWorkData(WorkItem* item, WorkData data)
{
	item->inputData = data.inputData;
	item->outputData = data.outputData;
}
	
void SetWorkFunc(WorkItem* item, WorkFunc* func, Finalizer* finalizer)
{
	item->func = func;
	item->finalizer = finalizer;
}

ThreadReturn ThreadCall WorkerThreadFunc(ThreadContext context);

YU_THREAD_LOCAL WorkerThread* worker;

WorkerThread* GetWorkerThread()
{
	return worker;
}

struct RenderQueue*	GetThreadLocalRenderQueue()
{
	return worker->renderQueue;
}

int GetWorkerThreadIdx()
{
	return worker->id;
}

void SubmitWorkItem(WorkItem* item, WorkItem* dep[], int numDep)
{
//	WorkerThread* workerThread = GetWorkerThread();
	item->permits = -numDep;

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
	Renderer* renderer = GetRenderer();
	worker->renderQueue = CreateRenderQueue(renderer);

	Log("Worker thread:%d started\n", worker->id);

	while (YuState() == YU_RUNNING)
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
	Renderer* renderer = GetRenderer();
	worker->renderQueue = CreateRenderQueue(renderer);
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
	gWorkerSystem->workItemIdList.Free();
}

void ResetWorkItem(WorkItem* item)
{
	item->permitList.Clear();
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

void InitWorkerSystem(Allocator* allocator)
{

	gWorkerSystem = DeepNewAligned<WorkerSystem>(allocator, CACHE_LINE);
	size_t workerSysSize = sizeof(*gWorkerSystem);

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

void FreeWorkerSystem(Allocator* allocator)
{
	DeleteAligned(allocator, gWorkerSystem);
}

}
