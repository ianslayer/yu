#include "../core/worker.h"
#include "work_map.h"

namespace yu
{

WorkMap workMap;
WorkMap* gWorkMap = &workMap;

WorkItem* FrameStartWorkItem();
WorkItem* FrameEndWorkItem();
WorkItem* InputWorkItem();
WorkItem* TestRenderItem();

void FreeWorkItem(WorkItem* item);

void Clear(WorkMap* workMap)
{
	ResetWorkItem(workMap->startWork);
	ResetWorkItem(workMap->inputWork);
	ResetWorkItem(workMap->testRenderer);
	ResetWorkItem(workMap->endWork);
}

void SubmitWork(WorkMap* workMap)
{
	SubmitWorkItem(workMap->startWork, nullptr, 0);
	SubmitWorkItem(workMap->inputWork, &workMap->startWork, 1);
	SubmitWorkItem(workMap->testRenderer, &workMap->inputWork, 1);

	WorkItem* endDep[2] = { workMap->startWork, workMap->testRenderer };

	SubmitWorkItem(workMap->endWork, endDep, 2);

}

void InitWorkMap()
{
	gWorkMap->startWork = FrameStartWorkItem();
	gWorkMap->inputWork = InputWorkItem();
	gWorkMap->endWork = FrameEndWorkItem();
	gWorkMap->testRenderer = TestRenderItem();
}

void FreeWorkMap()
{
	FreeWorkItem(gWorkMap->startWork);
	FreeWorkItem(gWorkMap->endWork);
	FreeWorkItem(gWorkMap->inputWork);
	FreeWorkItem(gWorkMap->testRenderer);
}

}