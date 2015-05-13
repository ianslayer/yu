#ifndef YU_WORKER
#define YU_WORKER

namespace yu
{

struct WorkItemHandle{ i32 id; };

struct InputData
{

};

struct OutputData
{
};

struct WorkItem;

WorkItemHandle	NewWorkItem();
void			FreeWorkItem(WorkItemHandle item);
WorkItem*		GetWorkItem(WorkItemHandle handle);

const InputData*  GetInputData(WorkItem* item);

void		SetInputData(WorkItem* item, InputData* input);

OutputData*	GetOutputData(WorkItem* item);
void		SetOutputData(WorkItem* item, OutputData* output);
int			GetNumDepend(WorkItem* item);
OutputData* GetDependOutputData(WorkItem* item, int i);

typedef void WorkFunc(WorkItem* item);
typedef void Finalizer(WorkItem* item);

void		SetWorkFunc(WorkItem* item, WorkFunc* func, Finalizer* finalizer);

void		SubmitWorkItem(WorkItem* item, WorkItem* dep[], int numDep);
bool		IsDone(WorkItem*);
void		ResetWorkItem(WorkItem* item);

struct WorkerThread* GetWorkerThread();
int					GetWorkerThreadIdx();

void MainThreadWorker();
void InitWorkerSystem(Allocator* allocator);
void FreeWorkerSystem(Allocator* allocator);
void SubmitTerminateWork();
}

#endif
