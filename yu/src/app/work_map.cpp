#include "../core/platform.h"
#include "../core/worker.h"
#include "work_map.h"
#include "../renderer/renderer.h"

namespace yu
{

WorkMap workMap;
WorkMap* gWorkMap = &workMap;

FrameWorkItemResult FrameWorkItem();
WorkItem* InputWorkItem();
WorkItem* TestRenderItem();
CameraHandle GetCamera(WorkItem* renderItem);
RenderQueue* GetRenderQueue(WorkItem* renderItem);
WorkItem* CameraControlItem(WorkItem* inputWorkItem, RenderQueue* queue, CameraHandle camHandle);

void FreeWorkItem(WorkItem* item);

void Clear(WorkMap* _workMap)
{
	ResetWorkItem(_workMap->startWork);
	ResetWorkItem(_workMap->inputWork);
	ResetWorkItem(_workMap->cameraControllerWork);
	ResetWorkItem(_workMap->testRenderer);
	ResetWorkItem(_workMap->endWork);
}

void SubmitWork(WorkMap* _workMap)
{
	SubmitWorkItem(_workMap->startWork, nullptr, 0);
	SubmitWorkItem(_workMap->inputWork, &_workMap->startWork, 1);
	SubmitWorkItem(_workMap->cameraControllerWork, &_workMap->inputWork, 1);
	SubmitWorkItem(_workMap->testRenderer, &_workMap->cameraControllerWork, 1);

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
	gWorkMap->cameraControllerWork = CameraControlItem(gWorkMap->inputWork, GetRenderQueue(gWorkMap->testRenderer), GetCamera(gWorkMap->testRenderer));
}

void FreeWorkMap()
{
	FreeSysWorkItem(gWorkMap->startWork);
	FreeSysWorkItem(gWorkMap->endWork);
	FreeSysWorkItem(gWorkMap->inputWork);
	FreeSysWorkItem(gWorkMap->cameraControllerWork);
	FreeSysWorkItem(gWorkMap->testRenderer);
}

}