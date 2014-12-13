#include "../core/platform.h"
#include "../core/thread.h"
#include "../core/worker.h"
#include "../core/allocator.h"
#include "../core/log.h"
#include "../core/system.h"
#include "../core/timer.h"
#include "../container/dequeue.h"
#include "../renderer/renderer.h"
#include "work_map.h"
#include <new>

namespace yu
{

struct FrameLockData : public InputData
{
	FrameLock*	frameLock;
};

std::atomic<bool> inFrame; //ugly ugly...

void FrameStart(FrameLock* lock)
{	
	WaitForKick(lock);
	inFrame = true;
}

void FrameEnd(FrameLock* lock)
{	
	FrameComplete(lock);
	inFrame = false;
}

bool WorkerFrameComplete()
{
	return (inFrame == false);
}


//----glue code----
void FrameStartWorkFunc(WorkItem* item)
{
	FrameLockData* lock = (FrameLockData*)GetInputData(item);
	FrameStart(lock->frameLock);
	//Log("frame: %ld started\n", frame);
}

void FrameEndWorkFunc(WorkItem* item)
{	
	FrameLockData* lock = (FrameLockData*)GetInputData(item);
	FrameEnd(lock->frameLock);
	//Log("frame: %ld ended\n", frame);
}

FrameWorkItemResult FrameWorkItem()
{
	FrameWorkItemResult item;
	FrameLock* frameLock = AddFrameLock();
	FrameLockData* frameLockData = New<FrameLockData>(gSysArena);
	frameLockData->source = gSysArena;
	frameLockData->frameLock = frameLock;

	item.frameStartItem = NewWorkItem(gSysArena);
	item.frameEndItem = NewWorkItem(gSysArena);
	
	SetWorkFunc(item.frameStartItem, FrameStartWorkFunc);
	SetWorkFunc(item.frameEndItem, FrameEndWorkFunc);
	SetInputData(item.frameStartItem, frameLockData);
	SetInputData(item.frameEndItem, frameLockData);

	return item;
}

#define MAX_INPUT 256
struct Input : public InputData
{
	SpscFifo<InputEvent, 256>* inputQueue;
};

struct Output : public OutputData
{
	InputEvent	frameEvent[MAX_INPUT];
	int			numEvent;
};

void ProcessInput(Input& input, Output& output)
{
	output.numEvent = 0;
	InputEvent event;

	while (input.inputQueue->Dequeue(event) && output.numEvent < MAX_INPUT)
	{
		Time sysInitTime = SysStartTime();
		Time eventTime;
		eventTime.time = event.timeStamp;

		output.frameEvent[output.numEvent++] = event;

		switch (event.type)
		{
		case InputEvent::UNKNOWN:
		break;
		case InputEvent::KEYBOARD:
		{
			Log("event time: %f ", ConvertToMs(eventTime - sysInitTime) / 1000.f);
			if (event.keyboardEvent.type == InputEvent::KeyboardEvent::DOWN)
			{
				Log("key down\n");
			}
			else if (event.keyboardEvent.type == InputEvent::KeyboardEvent::UP)
			{
				Log("key up\n");
			}
		}break;
		case InputEvent::MOUSE:
		{
			if (event.mouseEvent.type == InputEvent::MouseEvent::L_BUTTON_DOWN)
			{
				Log("event time: %f ", ConvertToMs(eventTime - sysInitTime) / 1000.f);
				Log("left button down: %f, %f\n", event.mouseEvent.x, event.mouseEvent.y);
			}
			if (event.mouseEvent.type == InputEvent::MouseEvent::R_BUTTON_DOWN)
			{
				Log("event time: %f ", ConvertToMs(eventTime - sysInitTime) / 1000.f);
				Log("right button down: %f, %f\n", event.mouseEvent.x, event.mouseEvent.y);
			}
			if (event.mouseEvent.type == InputEvent::MouseEvent::MOVE)
			{
			//	Log("event time: %f ", ConvertToMs(eventTime - sysInitTime) / 1000.f);
			//	Log("mouse location: %f, %f\n", event.mouseEvent.x, event.mouseEvent.y);
			}
			if (event.mouseEvent.type == InputEvent::MouseEvent::WHEEL)
			{
				Log("event time: %f ", ConvertToMs(eventTime - sysInitTime) / 1000.f);
				Log("scroll: %f\n", event.mouseEvent.scroll);
			}
		}break;
		}
	}
}


//-----glue code------
void InputWorkFunc(WorkItem* item)
{
	Output* output = (Output*)GetOutputData(item);
	Input* input = (Input*)GetInputData(item);
	ProcessInput(*input, *output);
}

WorkItem* InputWorkItem()
{
	WorkItem* item = NewWorkItem(gSysArena);
	SetWorkFunc(item, InputWorkFunc);
	Output* output = New<Output>(gSysArena);
	output->source = gSysArena;
	Input* input = New<Input>(gSysArena);
	input->source = gSysArena;
	input->inputQueue = (SpscFifo<InputEvent, 256>*) gSystem->GetInputQueue();
	SetInputData(item, input);
	SetOutputData(item, output);
	return item;
}

struct TestRenderer : public InputData
{
	Renderer* renderer;
	CameraHandle camera;
	MeshHandle	triangle;
	MeshHandle square;
	RenderQueue* queue;
};

struct FirstPersonControllerInput : public InputData
{
	CameraHandle camera;
};

struct FirstPersonControllerOutput : public OutputData
{
	CameraData camData;
};

void Render(TestRenderer* render)
{
	/*
	MeshData updateData = {};
	updateData.channelMask = MeshData::POSITION | MeshData::COLOR;
	Vector3 trianglePos[3] = { Vector3(0, 0.5, 0.5), Vector3(0.5, -0.5, 0.5), Vector3(-0.5, -0.5, 0.5) };
	Color color[3] = {};
	u32 indices[3] = { 0, 1, 2 };

	updateData.posList = trianglePos;
	updateData.colorList = color;
	updateData.indices = indices;
	updateData.numVertices = 3;
	updateData.numIndices = 3;

	UpdateMesh(render->queue, render->mesh, 0, 0, &updateData);
	*/
	CameraData camData = GetCameraData(render->queue, render->camera);
	camData.SetXFov(3.14f / 2.f, 1280, 720);

	UpdateCamera(render->queue, render->camera, camData);
	//Render(render->queue, render->camera, render->triangle);
	Render(render->queue, render->camera, render->square);
	Flush(render->queue);
	Swap(render->queue, true);
}

void TestRender(WorkItem* item)
{
	TestRenderer* renderer = (TestRenderer*) GetInputData(item);
	Render(renderer);
}

void FreeTestRender(WorkItem* item)
{
	TestRenderer* renderer = (TestRenderer*)GetInputData(item);
	FreeRenderer(renderer->renderer);
}

WorkItem* TestRenderItem()
{
	WorkItem* item = NewWorkItem(gSysArena);
	SetWorkFunc(item, TestRender);
	SetFinalizer(item, FreeTestRender);
	Renderer* renderer = CreateRenderer();
	TestRenderer* testRenderer = New<TestRenderer>(gSysArena);
	testRenderer->source = gSysArena;
	testRenderer->renderer = renderer;
	testRenderer->queue = CreateRenderQueue(renderer);

	SetInputData(item, testRenderer);

	testRenderer->triangle = CreateMesh(testRenderer->queue, 3, 3, MeshData::POSITION | MeshData::COLOR);
	MeshData triangleData = {};
	triangleData.channelMask = MeshData::POSITION | MeshData::COLOR;
	Vector3 trianglePos[3] = { Vector3(0, 0.5, 0.5), Vector3(0.5, -0.5, 0.5), Vector3(-0.5, -0.5, 0.5) };
	Color color[3] = {};
	u32 indices[3] = {0, 1, 2};

	triangleData.posList = trianglePos;
	triangleData.colorList = color;
	triangleData.indices = indices;
	triangleData.numVertices = 3;
	triangleData.numIndices = 3;

	UpdateMesh(testRenderer->queue, testRenderer->triangle, 0, 0, &triangleData);

	testRenderer->square = CreateMesh(testRenderer->queue, 4, 6, MeshData::POSITION | MeshData::COLOR);
	MeshData squareData = {};
	squareData.channelMask = MeshData::POSITION | MeshData::COLOR;
	Vector3 squarePos[4] = { Vector3(-10.f, -10.f, -5.f), Vector3(-10.f, 10.f, -5.f), Vector3(10.f, -10.f, -5.f), Vector3(10.f, 10.f, -5.f) };
	Color squareColor[4] = {};
	u32 squareIndices[6] = { 0, 1, 2, 2, 1, 3};

	squareData.posList = squarePos;
	squareData.colorList = squareColor;
	squareData.indices = squareIndices;
	squareData.numVertices = 4;
	squareData.numIndices = 6;
	UpdateMesh(testRenderer->queue, testRenderer->square, 0, 0, &squareData);

	testRenderer->camera = CreateCamera(testRenderer->queue);
	CameraData camData = CameraData();
	camData.position = Vector3(20, 0, 0.f);
	
	camData.SetXFov(3.14f / 2.f, 1280, 720);
	UpdateCamera(testRenderer->queue, testRenderer->camera, camData);

	Matrix4x4 viewProjMatrix = camData.PerspectiveMatrix() * camData.ViewMatrix();
	Matrix4x4 viewMatrix = camData.ViewMatrix();
	Vector4 viewPos[4];
	Vector4 projPos[4];
	for (int i = 0; i < 4; i++)
	{
		viewPos[i] = viewMatrix * Vector4(squarePos[i], 1);
		projPos[i] = viewProjMatrix * Vector4(squarePos[i], 1);
		projPos[i] /= projPos[i].w;
	}

	return item;
}

}