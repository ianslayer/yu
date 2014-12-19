#include "../core/platform.h"
#include "../core/thread.h"
#include "../core/worker.h"
#include "../core/allocator.h"
#include "../core/log.h"
#include "../core/system.h"
#include "../core/timer.h"
#include "../container/dequeue.h"
#include "../renderer/renderer.h"
#include "../renderer/shader.h"


#include "work_map.h"
#include <new>

namespace yu
{

struct FrameLockData : public InputData
{
	DECLARE_INPUT_TYPE(FrameLockData)
	FrameLock*	frameLock;
};
IMP_INPUT_TYPE(FrameLockData)

void RegisterFrameLockType()
{
	FrameLockData::typeList = InputData::typeList;
	InputData::typeList = FrameLockData::typeList;
}
//IMP_INPUT_TYPE(FrameLockData)

void FrameStart(FrameLock* lock)
{	
	WaitForKick(lock);
}

void FrameEnd(FrameLock* lock)
{	
	FrameComplete(lock);
}

bool WorkerFrameComplete()
{
	return IsDone(gWorkMap->endWork);
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
	frameLockData->frameLock = frameLock;

	item.frameStartItem = NewSysWorkItem();
	item.frameEndItem = NewSysWorkItem();
	
	SetWorkFunc(item.frameStartItem, FrameStartWorkFunc, nullptr);
	SetWorkFunc(item.frameEndItem, FrameEndWorkFunc, nullptr);
	SetInputData(item.frameStartItem, frameLockData);
	SetInputData(item.frameEndItem, frameLockData);

	return item;
}

#define MAX_INPUT 256
#define MAX_KEYBOARD_STATE 256
struct Input : public InputData
{
	SpscFifo<InputEvent, 256>* inputQueue;
};

struct KeyState
{
	u32 pressCount;
	u32 pressed;
};

struct MouseState
{
	float x, y;
	float dx, dy;
	bool	 captured;
	KeyState leftButton;
	KeyState rightButton;
};

struct InputResult : public OutputData
{
	InputEvent	frameEvent[MAX_INPUT];
	KeyState	keyboardState[MAX_KEYBOARD_STATE];
	MouseState	mouseState;
	int			numEvent;
};

void ProcessInput(Input& input, InputResult& output)
{
//	int	threadIdx =	GetWorkerThreadIdx();
//	Log("thread id:%d\n", threadIdx);

	output.numEvent = 0;
	InputEvent event;

	for (int i = 0; i < MAX_KEYBOARD_STATE; i++)
	{
		output.keyboardState[i].pressCount = 0;
	}

	output.mouseState.leftButton.pressCount = 0;
	output.mouseState.rightButton.pressCount = 0;
	output.mouseState.dx = output.mouseState.dy = 0.f;

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
			//Log("event time: %f ", ConvertToMs(eventTime - sysInitTime) / 1000.f);
			if (event.keyboardEvent.type == InputEvent::KeyboardEvent::DOWN)
			{
				//Log("%c key down\n", event.keyboardEvent.key);
				output.keyboardState[event.keyboardEvent.key].pressCount++;
				output.keyboardState[event.keyboardEvent.key].pressed = 1;
			}
			else if (event.keyboardEvent.type == InputEvent::KeyboardEvent::UP)
			{
				//Log("%c key up\n", event.keyboardEvent.key);
				output.keyboardState[event.keyboardEvent.key].pressed = 0;
			}
		}break;
		case InputEvent::MOUSE:
		{
			output.mouseState.captured = event.window.captured;
			if (event.mouseEvent.type == InputEvent::MouseEvent::L_BUTTON_DOWN)
			{
				output.mouseState.leftButton.pressCount++;
				output.mouseState.leftButton.pressed = 1;
				//Log("event time: %f ", ConvertToMs(eventTime - sysInitTime) / 1000.f);
				//Log("left button down: %f, %f\n", event.mouseEvent.x, event.mouseEvent.y);
			}
			else if (event.mouseEvent.type == InputEvent::MouseEvent::L_BUTTON_UP)
			{
				output.mouseState.leftButton.pressed = 0;
			}

			if (event.mouseEvent.type == InputEvent::MouseEvent::R_BUTTON_DOWN)
			{
				output.mouseState.rightButton.pressCount++;
				output.mouseState.rightButton.pressed = 1;
				//Log("event time: %f ", ConvertToMs(eventTime - sysInitTime) / 1000.f);
				//Log("right button down: %f, %f\n", event.mouseEvent.x, event.mouseEvent.y);
			}
			else if (event.mouseEvent.type == InputEvent::MouseEvent::R_BUTTON_UP)
			{
				output.mouseState.rightButton.pressed = 0;
			}


			if (event.mouseEvent.type == InputEvent::MouseEvent::MOVE)
			{
				
				if (event.window.mode == Window::MOUSE_FREE)
				{
					output.mouseState.dx += (event.mouseEvent.x - output.mouseState.x);
					output.mouseState.x = event.mouseEvent.x;

					output.mouseState.dy += (event.mouseEvent.y - output.mouseState.y);
					output.mouseState.y = event.mouseEvent.y;

				}
				else if (event.window.mode == Window::MOUSE_CAPTURE)
				{
					output.mouseState.dx += event.mouseEvent.x;
					output.mouseState.x = 0;

					output.mouseState.dy += event.mouseEvent.y;
					output.mouseState.y = 0;
				}
				//Log("event time: %f ", ConvertToMs(eventTime - sysInitTime) / 1000.f);
				//Log("mouse location: %f, %f\n", event.mouseEvent.x, event.mouseEvent.y);
				//Log("mouse dx dy: %f, %f\n", output.mouseState.dx, output.mouseState.dy);
			}
			if (event.mouseEvent.type == InputEvent::MouseEvent::WHEEL)
			{
				//Log("event time: %f ", ConvertToMs(eventTime - sysInitTime) / 1000.f);
				//Log("scroll: %f\n", event.mouseEvent.scroll);
			}

		}break;
		}
	}
}


//-----glue code------
void InputWorkFunc(WorkItem* item)
{
	InputResult* result = (InputResult*)GetOutputData(item);
	Input* input = (Input*)GetInputData(item);
	ProcessInput(*input, *result);
}

WorkItem* InputWorkItem()
{
	WorkItem* item = NewSysWorkItem();
	SetWorkFunc(item, InputWorkFunc, nullptr);
	InputResult* result = New<InputResult>(gSysArena);
	memset(result->keyboardState, 0, sizeof(result->keyboardState));
	memset(&result->mouseState, 0, sizeof(result->mouseState));
	Input* input = New<Input>(gSysArena);
	input->inputQueue = (SpscFifo<InputEvent, 256>*) gSystem->GetInputQueue();
	SetInputData(item, input);
	SetOutputData(item, result);
	return item;
}

struct CameraControllerInput : public InputData
{
	InputResult* inputResult;
	RenderQueue* queue;
	CameraHandle camera;
	float		moveSpeed;
	float		turnSpeed;
};


struct CameraControllerOutput : public OutputData
{
	CameraData		updatedCamera;
};

void CameraControl(const CameraControllerInput& input, CameraControllerOutput& output)
{
	//int	threadIdx = GetWorkerThreadIdx();
	//Log("thread id:%d\n", threadIdx);

	CameraData data = GetCameraData(input.queue, input.camera);

	Vector3 lookAt = data.lookat;
	Vector3 right = data.right;
	Vector3 position = data.position;
	Vector3 up = -data.Down();

	if (input.inputResult->mouseState.captured || input.inputResult->mouseState.rightButton.pressed == 1)
	{
		Matrix3x3 yaw = Matrix3x3::RotateAxis(up, -input.inputResult->mouseState.dx * input.turnSpeed);
		data.lookat = yaw * lookAt;
		data.right = yaw * right;

		Matrix3x3 pitch = Matrix3x3::RotateAxis(right, -input.inputResult->mouseState.dy * input.turnSpeed);
		data.lookat = pitch * data.lookat;

		data.right.z = 0;
		data.right.Normalize();
		data.lookat = cross(data.right, data.Down());
		data.lookat.Normalize();
	}

	lookAt = data.lookat;
	right = data.right;

	if (input.inputResult->keyboardState['W'].pressed == 1)
	{
		position += lookAt * input.moveSpeed;	
	}
	if (input.inputResult->keyboardState['S'].pressed == 1)
	{
		position -= lookAt * input.moveSpeed;
	}
	if (input.inputResult->keyboardState['D'].pressed == 1)
	{
		position += right * input.moveSpeed;
	}
	if (input.inputResult->keyboardState['A'].pressed == 1)
	{
		position -= right * input.moveSpeed;
	}


	data.position = position;
	UpdateCamera(input.queue, input.camera, data);
	output.updatedCamera = data;

}

void CameraControlWorkItem(WorkItem* item)
{
	CameraControllerInput* input = (CameraControllerInput*)GetInputData(item);
	CameraControllerOutput* output = (CameraControllerOutput*)GetOutputData(item);
	CameraControl(*input, *output);
}

WorkItem* CameraControlItem(WorkItem* inputWorkItem, RenderQueue* queue, CameraHandle camHandle)
{
	InputResult* result = (InputResult*)GetOutputData(inputWorkItem);

	WorkItem* item = NewSysWorkItem();

	CameraControllerInput* input = New<CameraControllerInput>(gSysArena);
	input->inputResult = result;
	input->queue = queue;
	input->camera = camHandle;
	input->moveSpeed = 0.1f;
	input->turnSpeed = 0.01f;
	CameraControllerOutput* output = New<CameraControllerOutput>(gSysArena);
	SetWorkFunc(item, CameraControlWorkItem, nullptr);
	SetInputData(item, input);
	SetOutputData(item, output);
	return item;
}


struct TestRenderer : public InputData
{
	Renderer* renderer;
	CameraHandle camera;

	CameraControllerOutput* updatedCam;

	MeshHandle	triangle;
	MeshHandle square;
	RenderQueue* queue;

	PipelineHandle		pipeline;
	VertexShaderHandle	vs;
	PixelShaderHandle	ps;
};

CameraHandle GetCamera(WorkItem* renderItem)
{
	TestRenderer* renderer = (TestRenderer*)GetInputData(renderItem);
	return renderer->camera;
}

RenderQueue* GetRenderQueue(WorkItem* renderItem)
{
	TestRenderer* renderer = (TestRenderer*)GetInputData(renderItem);
	return renderer->queue;
}

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
	
	//CameraData camData = GetCameraData(render->queue, render->camera);
	//camData.SetXFov(3.14f / 2.f, 1280, 720);
	//camData.position = _Vector3(50, 0, 0.f);
	//UpdateCamera(render->queue, render->camera, camData);

	Render(render->queue, render->camera, render->triangle, render->pipeline);
	//for (int i = 0; i < 1000; i++)
	Render(render->queue, render->camera, render->square, render->pipeline);
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
	WorkItem* item = NewSysWorkItem();
	SetWorkFunc(item, TestRender, FreeTestRender);
	Renderer* renderer = CreateRenderer();
	TestRenderer* testRenderer = New<TestRenderer>(gSysArena);
	testRenderer->renderer = renderer;
	testRenderer->queue = CreateRenderQueue(renderer);

	SetInputData(item, testRenderer);

	testRenderer->triangle = CreateMesh(testRenderer->queue, 3, 3, MeshData::POSITION | MeshData::COLOR);
	MeshData triangleData = {};
	triangleData.channelMask = MeshData::POSITION | MeshData::COLOR;
	Vector3 trianglePos[3] = { _Vector3(0, 5, 5), _Vector3(5, -5, 5), _Vector3(-5, -5, 5) };
	Color color[3] = {};
	u32 indices[3] = {0, 2, 1};

	triangleData.posList = trianglePos;
	triangleData.colorList = color;
	triangleData.indices = indices;
	triangleData.numVertices = 3;
	triangleData.numIndices = 3;

	UpdateMesh(testRenderer->queue, testRenderer->triangle, 0, 0, &triangleData);

	testRenderer->square = CreateMesh(testRenderer->queue, 4, 6, MeshData::POSITION | MeshData::COLOR);
	MeshData squareData = {};
	squareData.channelMask = MeshData::POSITION | MeshData::COLOR;
	Vector3 squarePos[4] = { _Vector3(-10.f, -10.f, -5.f), _Vector3(-10.f, 10.f, -5.f), _Vector3(10.f, -10.f, -5.f), _Vector3(10.f, 10.f, -5.f) };
	Color squareColor[4] = {};
	u32 squareIndices[6] = { 0, 1, 2, 2, 1, 3};

	squareData.posList = squarePos;
	squareData.colorList = squareColor;
	squareData.indices = squareIndices;
	squareData.numVertices = 4;
	squareData.numIndices = 6;
	UpdateMesh(testRenderer->queue, testRenderer->square, 0, 0, &squareData);

	VertexShaderAPIData vsData = CompileVSFromFile("data/shaders/flat_vs.hlsl");
	PixelShaderAPIData psData = CompilePSFromFile("data/shaders/flat_ps.hlsl");

	testRenderer->vs = CreateVertexShader(testRenderer->queue, vsData);
	testRenderer->ps = CreatePixelShader(testRenderer->queue, psData);
	PipelineData pipelineData;
	pipelineData.vs = testRenderer->vs;
	pipelineData.ps = testRenderer->ps;

	testRenderer->pipeline = CreatePipeline(testRenderer->queue, pipelineData);


	testRenderer->camera = CreateCamera(testRenderer->queue);
	CameraData camData = DefaultCamera();
	camData.position = _Vector3(50, 0, 0.f);
	
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