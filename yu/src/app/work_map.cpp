#include "../core/platform.h"
#include "../core/worker.h"
#include "work_map.h"

namespace yu
{

WorkMap workMap;
WorkMap* gWorkMap = &workMap;

FrameWorkItemResult FrameWorkItem();
WorkItem* InputWorkItem();
WorkItem* TestRenderItem();

void FreeWorkItem(WorkItem* item);

void Clear(WorkMap* _workMap)
{
	ResetWorkItem(_workMap->startWork);
	ResetWorkItem(_workMap->inputWork);
	ResetWorkItem(_workMap->testRenderer);
	ResetWorkItem(_workMap->endWork);
}

void SubmitWork(WorkMap* _workMap)
{
	SubmitWorkItem(_workMap->startWork, nullptr, 0);
	SubmitWorkItem(_workMap->inputWork, &_workMap->startWork, 1);
	SubmitWorkItem(_workMap->testRenderer, &_workMap->inputWork, 1);

	WorkItem* endDep[1] = { _workMap->testRenderer };

	SubmitWorkItem(_workMap->endWork, endDep, 1);

}

void InitWorkMap()
{
	FrameWorkItemResult frameWork = FrameWorkItem();
	gWorkMap->startWork = frameWork.frameStartItem;
	gWorkMap->inputWork = InputWorkItem();
	gWorkMap->endWork = frameWork.frameEndItem;
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