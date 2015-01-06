namespace yu
{
#define DECLARE_INPUT_TYPE(typeName)  static InputData* typeList;
#define IMP_INPUT_TYPE(typeName) InputData* typeName::typeList;

struct InputData
{
	DECLARE_INPUT_TYPE(InputData)
};

struct OutputData
{
};

struct WorkItem;
WorkItem*	NewSysWorkItem();
void		FreeSysWorkItem(WorkItem*);

WorkItem*	NewFrameWorkItem();

InputData*	GetInputData(WorkItem* item);
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
void InitWorkerSystem();
void FreeWorkerSystem();
void SubmitTerminateWork();
}