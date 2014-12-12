namespace yu
{
//TODO: need a fast per frame work item pool
struct WorkItem;
class Allocator;

struct InputData
{
	Allocator* source = nullptr;
};

struct OutputData
{
	Allocator* source = nullptr;
};

typedef void WorkFunc(WorkItem* item);
typedef void Finalizer(WorkItem* item);

WorkItem*	NewWorkItem(Allocator* allocator);
void		FreeWorkItem(WorkItem*);

InputData*	AllocInputData(WorkItem* item, size_t size);
InputData*	GetInputData(WorkItem* item);
void		SetInputData(WorkItem* item, InputData* input);
void		FreeInputData(WorkItem* item);

OutputData* AllocOutputData(WorkItem* item, size_t size);
OutputData*	GetOutputData(WorkItem* item);
void		SetOutputData(WorkItem* item, OutputData* output);
int			GetNumDepend(WorkItem* item);
OutputData* GetDependOutputData(WorkItem* item, int i);
void		FreeOutputData(WorkItem* item);

void		SetWorkFunc(WorkItem* item, WorkFunc* func);
void		SetFinalizer(WorkItem* item, Finalizer* func);

void		SubmitWorkItem(WorkItem* item, WorkItem* dep[], int numDep);
bool		IsDone(WorkItem*);
void		ResetWorkItem(WorkItem* item);

void		SubmitTerminateWork();

void MainThreadWorker();
void InitWorkerSystem();
void FreeWorkerSystem();

}