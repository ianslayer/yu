
#include "worker.h"
#include "free_list.h"
#include "handle.h"
#include "../math/yu_math.h"
#include "../yu.h"

namespace yu
{

#define MAX_WORK_QUEUE_LEN 1024
#define MAX_WORK_ITEM 4096
	
struct WorkItem;
struct WorkItemLink
{
	WorkItem* item;
	WorkItemLink* next;
};

struct InternalWorkHandle
{
	i32					id;
	std::atomic<i32>	seq;

	//FUCK c++ 11
	InternalWorkHandle& operator=(const InternalWorkHandle& rhs)
	{
		id = rhs.id;
		seq = rhs.seq.load();
		return *this;
	}
};

inline WorkHandle ToWorkHandle(const InternalWorkHandle& internalHandle)
{
	WorkHandle handle = {internalHandle.id, internalHandle.seq.load()};
	return handle;
}

struct WorkItem
{
	WorkItem() :  func(nullptr), finalizer(nullptr)
	{
		permits = 0;
		isDone = false;

		nextPermit = nullptr;
		
		data.userData = nullptr;
		data.completeData = nullptr;
		
		DEBUG_ONLY(dbgFrameCount = 0);
	}

	~WorkItem()
	{
		//TODO: where should we release the data ?
		//Allocator* allocator = dependList.GetAllocator();
	}

	InternalWorkHandle 		handle;
	std::atomic<i32>		refCount;
	
	WorkItemLink*			nextPermit;

	Mutex					lock;

	WorkFunc*				func;
	Finalizer*				finalizer;

	WorkData				data;

	std::atomic<int>		permits;
	std::atomic<bool>		isDone;

#if defined YU_DEBUG
	std::atomic<u64>		dbgFrameCount;
#endif
} ;

struct WorkerThread
{
	int						id = -1;
	Thread					thread;

	Array<WorkHandle>	retiredItemPermitList; //TODO: move this into retired function, this should use a stack allocator (alloca)
};

void TerminateWorkFunc(WorkHandle work) {}
struct WorkerSystem
{
	WorkerSystem() :workQueueSem(0, MAX_WORK_QUEUE_LEN)
	{
		CPUInfo cpuInfo = SystemInfo::GetCPUInfo();
		numWorkerThread = yu::max((i32)cpuInfo.numLogicalProcessors - (i32)2, (i32)0);

		workerThread = new WorkerThread[numWorkerThread + 1];

		workItemList = new WorkItem[MAX_WORK_ITEM];
		permitLinkList = new WorkItemLink[MAX_WORK_ITEM];

		for(int i = 0; i < MAX_WORK_ITEM-1; i++)
		{
			workItemList[i].handle = {i, -1};
			permitLinkList[i].next = &permitLinkList[i+1];
		}
	}

	WorkerThread*			workerThread; //0 is main thread
	unsigned int			numWorkerThread = 0;		

	MpmcFifo<WorkHandle,	MAX_WORK_QUEUE_LEN>	workQueue;
	Semaphore				workQueueSem;

	WorkHandle				terminateWorkItem;

	HandleFreeList<MAX_WORK_ITEM> workItemHandleList;	
	WorkItem*				workItemList;
	
	std::atomic<WorkItemLink*>	permitLinkList;
};

YU_GLOBAL WorkerSystem* gWorkerSystem;

/*
bool WorkAlive(WorkHandle work)
{
	
}

WorkItem* TryLock(WorkHandle work)
{
}

*/

WorkItemLink* NewPermitLink()
{	
	WorkItemLink* head = gWorkerSystem->permitLinkList;
	WorkItemLink* next = head->next;
	assert(head && next);
  
	while(!std::atomic_compare_exchange_weak(&gWorkerSystem->permitLinkList, &head, next))
	{
	 	head = gWorkerSystem->permitLinkList;
		next = head->next;
	}
	
	return head;
}

void FreePermitLink(WorkItemLink* link)
{
	WorkItemLink* head = gWorkerSystem->permitLinkList;
	link->next = head;
	gWorkerSystem->permitLinkList = link;
	while(!std::atomic_compare_exchange_weak(&gWorkerSystem->permitLinkList, &link, link))
	{
		head = gWorkerSystem->permitLinkList;
		link->next = head;
		gWorkerSystem->permitLinkList = link;
	}
}

WorkItem* GetWorkItem(WorkHandle work)
{
	return gWorkerSystem->workItemList + work.id;
}

/*
WorkItem* LockWorkItem(WorkHandle work)
{
}
*/

WorkHandle NewWorkItem()
{
	WorkHandle workHandle;

	Handle handle = gWorkerSystem->workItemHandleList.Alloc();

	workHandle = ToTypedHandle<WorkHandle>(handle);
	
	gWorkerSystem->workItemList[workHandle.id].handle.id = workHandle.id;
	gWorkerSystem->workItemList[workHandle.id].handle.seq.store(workHandle.seq);
	return workHandle;
}

void FreeWorkItem(WorkHandle work)
{
	gWorkerSystem->workItemHandleList.Free(ToHandle(work));
}

/*
void RecycleWorkItem()
{
	int numFreeId =  gWorkerSystem->workItemIdList.numDeferredFreed;
	for(int i = 0; i < numFreeId; i++)
	{
		WorkHandle garbageWork = {gWorkerSystem->workItemIdList.deferredFreeIndices[i]};
		ResetWorkItem(garbageWork);
	}
	gWorkerSystem->workItemIdList.Free();
}
*/

WorkData GetWorkData(WorkHandle work)
{
	return GetWorkItem(work)->data;
}

void SetWorkData(WorkHandle work, WorkData data)
{
	GetWorkItem(work)->data  = data;
}
	
void SetWorkFunc(WorkHandle work, WorkFunc* func, Finalizer* finalizer)
{
	GetWorkItem(work)->func = func;
	GetWorkItem(work)->finalizer = finalizer;
}

ThreadReturn ThreadCall WorkerThreadFunc(ThreadContext context);

YU_THREAD_LOCAL WorkerThread* worker;

WorkerThread* GetWorkerThread()
{
	return worker;
}

int GetWorkerThreadIndex()
{
	return worker->id;
}

void SubmitWorkItem(WorkHandle work, WorkHandle depWork[], int numDep)
{
//	WorkerThread* workerThread = GetWorkerThread();
	WorkItem* item = GetWorkItem(work);
	item->permits = -numDep;

	if (numDep == 0)
	{
		gWorkerSystem->workQueue.Enqueue(work);
		SignalSem(gWorkerSystem->workQueueSem);
		return;
	}
	for (int i = 0; i < numDep; i++)
	{
		WorkItem* dep = GetWorkItem(depWork[i]);		
		ScopedLock lock(dep->lock);
		if (!dep->isDone.load(std::memory_order_acquire))
		{
			WorkItemLink* nextPermit = NewPermitLink();
			nextPermit->item = item;
			nextPermit->next = dep->nextPermit;
			dep->nextPermit = nextPermit;
		}
		else
		{
			int prev = item->permits.fetch_add(1);
			if (prev == -1)
			{
				gWorkerSystem->workQueue.Enqueue(work);
				SignalSem(gWorkerSystem->workQueueSem);
			}
		}

	}
}

void Complete(WorkHandle work, Array<WorkHandle>& permitList)
{

	WorkItem* item = GetWorkItem(work);
	if(item->data.completeData)
	{
		DEBUG_ONLY(item->data.completeData->dataValid = (true));
	}
	
	item->lock.Lock();
	item->isDone.store(true, std::memory_order_release);

	WorkItemLink* nextPermit = item->nextPermit;
	while(nextPermit)
	{
		permitList.PushBack(ToWorkHandle(nextPermit->item->handle));
		nextPermit = nextPermit->next;
	}
	
	item->lock.Unlock();

	for (int i = 0; i < permitList.Size(); i++)
	{
		WorkHandle permitWork = permitList[i];
		WorkItem* p = GetWorkItem(permitWork);
		int prev = p->permits.fetch_add(1);

		if (prev == -1)
		{
			gWorkerSystem->workQueue.Enqueue(permitWork);
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

	while (YuState() == YU_RUNNING)
	{
		WaitForSem(gWorkerSystem->workQueueSem); //TODO: eliminate gWorkerSystem here, should be passed in as parameter
		WorkHandle work;
		while (gWorkerSystem->workQueue.Dequeue(work))
		{
			WorkItem* item = GetWorkItem(work);
			item->func(work);
			worker->retiredItemPermitList.Clear();
			Complete(work, worker->retiredItemPermitList);
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
		WorkHandle work;
		while (gWorkerSystem->workQueue.Dequeue(work))
		{
			WorkItem* item = GetWorkItem(work);
			item->func(work);
			worker->retiredItemPermitList.Clear();
			Complete(work, worker->retiredItemPermitList);
		}
	}

	//RecycleWorkItem();
}

void ResetWorkItem(WorkHandle work)
{
	WorkItem* item = GetWorkItem(work);
	while(item->nextPermit)
	{
		WorkItemLink* freePermit = item->nextPermit;
		item->nextPermit = freePermit->next;
		FreePermitLink(freePermit);
	}
	
	if(item->data.completeData)
	{
		DEBUG_ONLY( item->data.completeData->dataValid = (false) );
	}
	
	item->permits = 0;
	item->isDone.store(false);
}

bool IsDone(WorkHandle work)
{
	WorkItem* item = GetWorkItem(work);
	return item->isDone.load(std::memory_order_acquire);
}


void SubmitTerminateWork()
{
	for (unsigned int i = 0; i < gWorkerSystem->numWorkerThread; i++)
	{
		SubmitWorkItem(gWorkerSystem->terminateWorkItem, nullptr, 0);
	}
}

void InitWorkerSystem()
{
	gWorkerSystem = new WorkerSystem();
	gWorkerSystem->terminateWorkItem = NewWorkItem();
	SetWorkFunc(gWorkerSystem->terminateWorkItem, TerminateWorkFunc, nullptr);
	

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
	delete gWorkerSystem;
}

}
