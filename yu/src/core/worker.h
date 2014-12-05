namespace yu
{
//TODO: need a fast per frame work item pool
struct WorkItem;
typedef void WorkFunc(const void* input, void* output, const WorkItem* dep, int numDep);

WorkItem*	NewWorkItem();
void		FreeWorkItem(WorkItem* item);
void		SetWorkFunc(WorkItem* item, WorkFunc* func);
void		SetConstData(WorkItem* item, const void* data);
void		SetMutableData(WorkItem* item, void* data);

const void* GetPrevFrameOutput(WorkItem* item);
const void* GetCurFrameOutput(WorkItem* item, WorkItem* depItem); //note: must check dependency

void		SubmitWorkItem(WorkItem* item, WorkItem* dep, int numDep);
WorkItem*	FrameStartWorkItem();
void		SubmitTerminateWork();

void InitWorkerSystem();
void FreeWorkerSystem();

}