#ifndef YU_WORKER
#define YU_WORKER
#include "platform.h"
#include "handle.h"
#include <atomic>
namespace yu
{

struct WorkHandle
{
	i32 id;
	i32 seq;
};
	
struct CompleteData //after process, the data is not mutable anymore, so can be accessed by any threads
{
	DEBUG_ONLY(std::atomic<bool> dataValid);
};

template<class T>
struct CompleteDataRef
{
	CompleteDataRef() : p(nullptr) {};
	~CompleteDataRef() {};
	CompleteDataRef(T* _p) : p(_p) {};

	CompleteDataRef& operator=(T* _p)
	{
		p = _p;
		return *this;
	}
	
	T* operator->()
	{
		DEBUG_ONLY(assert(p->dataValid));
		return p;
	}
	
	const T* operator->() const
	{
		DEBUG_ONLY(assert(p->dataValid));
		return p;
	}
	
	T* p;
};

struct WorkData
{
	void*				userData;
	CompleteData*		completeData;
};

enum WorkLifeTime
{
	WORK_LIFE_TIME_FIRE_FORGET,
	WORK_LIFE_TIME_FRAME_FIRE_FORGET,
	WORK_LIFE_TIME_MANUAL_CONTROL,
};

WorkHandle		NewWorkItem(WorkLifeTime lifeTime);
WorkHandle		NewWorkItem();
void			FreeWorkItem(WorkHandle work);

WorkData	GetWorkData(WorkHandle work);
void		SetWorkData(WorkHandle work, WorkData data);

typedef void WorkFunc(WorkHandle work);
typedef void Finalizer(WorkHandle work);

void		SetWorkFunc(WorkHandle work, WorkFunc* func, Finalizer* finalizer);

void		SubmitWorkItem(WorkHandle work, WorkHandle dep[], int numDep);
bool		IsDone(WorkHandle work);

void		ResetWorkItem(WorkHandle work);

struct WorkerThread* GetWorkerThread();
int					GetWorkerThreadIndex();

void  AddEndFrameDependency(WorkHandle work);
	
//TODO:
//batch start
void	   SubmitWorkItems(WorkHandle items[], int numItems, WorkHandle* dep[], int numDep[]);
//yield, this must capture stack...
void	   YieldWork(WorkHandle self, WorkHandle* dep[], int numDep);

void MainThreadWorker();
void InitWorkerSystem();
void FreeWorkerSystem();
void SubmitTerminateWork();
}

#endif
