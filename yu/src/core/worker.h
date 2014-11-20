#include "thread.h"

namespace yu
{

struct WorkItem
{


};

class WorkerThread
{
	Thread thread;
	WorkItem perFramePool[1024]; //use thread local storage?
};

class WorkerSysem
{

};

//need a fast per frame work item pool

void SubmitWorkItem(WorkItem* item, WorkItem* dep, int numDep);

void InitWorkerSystem();
void FreeWorkerSystem();

}