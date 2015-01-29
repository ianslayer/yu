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
	FrameLock*	frameLock;
};

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

struct JoyState
{
	u32 buttonState;
	float leftX;
	float leftY;

	float rightX;
	float rightY;
};

struct InputResult : public OutputData
{
	InputEvent	frameEvent[MAX_INPUT];
	KeyState	keyboardState[MAX_KEYBOARD_STATE];
	MouseState	mouseState;
	JoyState	joyState[4];
	int			numEvent;
};

void ProcessInput(InputResult& output)
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

	while (gWindowManager->DequeueInputEvent(event) && output.numEvent < MAX_INPUT)
	{
		Time sysInitTime = SysStartTime();
		Time eventTime;
		eventTime.time = event.timeStamp;

		output.frameEvent[output.numEvent++] = event;

		switch (event.type)
		{
		case InputEvent::UNKNOWN:
		break;
		case InputEvent::KILL_FOCUS:
		{
			memset(&output, 0, sizeof(output));
		}
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
		case InputEvent::JOY:
		{
			if (event.joyEvent.type & InputEvent::JoyEvent::AXIS)
			{
				int controllerIdx = event.joyEvent.controllerIdx;
				output.joyState[controllerIdx].leftX = event.joyEvent.leftX;
				output.joyState[controllerIdx].leftY = event.joyEvent.leftY;
				output.joyState[controllerIdx].rightX = event.joyEvent.rightX;
				output.joyState[controllerIdx].rightY = event.joyEvent.rightY;
			}

			if (event.joyEvent.type & InputEvent::JoyEvent::BUTTON)
			{
				int controllerIdx = event.joyEvent.controllerIdx;
				output.joyState[controllerIdx].buttonState = event.joyEvent.buttonState;
			}

		}break;
		}
	}
}


//-----glue code------
void InputWorkFunc(WorkItem* item)
{
	InputResult* result = (InputResult*)GetOutputData(item);
	ProcessInput(*result);
}

WorkItem* InputWorkItem()
{
	WorkItem* item = NewSysWorkItem();
	SetWorkFunc(item, InputWorkFunc, nullptr);
	InputResult* result = New<InputResult>(gSysArena);
	memset(result, 0, sizeof(*result));
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
	Vector3 aheadDir = _Vector3(lookAt.x, lookAt.y, 0);
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
		position += aheadDir * input.moveSpeed;
	}
	if (input.inputResult->keyboardState['S'].pressed == 1)
	{
		position -= aheadDir * input.moveSpeed;
	}
	if (input.inputResult->keyboardState['D'].pressed == 1)
	{
		position += right * input.moveSpeed;
	}
	if (input.inputResult->keyboardState['A'].pressed == 1)
	{
		position -= right * input.moveSpeed;
	}

	//controller control
	{
		Matrix3x3 yaw = Matrix3x3::RotateAxis(up, -input.inputResult->joyState[0].rightX * input.turnSpeed * 2);
		data.lookat = yaw * lookAt;
		data.right = yaw * right;

		Matrix3x3 pitch = Matrix3x3::RotateAxis(right, input.inputResult->joyState[0].rightY * input.turnSpeed * 2);
		data.lookat = pitch * data.lookat;

		data.right.z = 0;
		data.right.Normalize();
		data.lookat = cross(data.right, data.Down());
		data.lookat.Normalize();

		position += aheadDir * input.moveSpeed * input.inputResult->joyState[0].leftY;
		position += right * input.moveSpeed * input.inputResult->joyState[0].leftX;
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

	MeshData triangleData = {};

	Vector3 trianglePos[3] ;
	Color triangleColor[3] ;
	u32 triangleIndices[3] ;

	MeshHandle	triangle;
	MeshHandle square;
	RenderQueue* queue;

	TextureMipData texData;
	Color		texels[16];
	RenderResource::TextureSlot textureSlot;

	PipelineHandle		pipeline;
	VertexShaderHandle	vs;
	PixelShaderHandle	ps;

	FenceHandle			createResourceFence;
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

void Render(TestRenderer* renderer)
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

	RenderResource resource = {};
	resource.numPsTexture = 1;
	resource.psTextures = &renderer->textureSlot;

	//WaitFence(renderer->queue, renderer->createResourceFence);

	//Reset(renderer->queue, renderer->createResourceFence);

	Render(renderer->queue, renderer->camera, renderer->triangle, renderer->pipeline, resource);
	
	InsertFence(renderer->queue, renderer->createResourceFence);

	//WaitFence(renderer->queue, renderer->createResourceFence);
	//for (int i = 0; i < 1000; i++)
	Render(renderer->queue, renderer->camera, renderer->square, renderer->pipeline, resource);
	Flush(renderer->queue);
	Swap(renderer->queue, true);
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

	testRenderer->createResourceFence = CreateFence(testRenderer->queue);

	testRenderer->triangle = CreateMesh(testRenderer->queue, 3, 3, MeshData::POSITION | MeshData::COLOR);

	testRenderer->trianglePos[0] = _Vector3(0, 5, 5);
	testRenderer->trianglePos[1] = _Vector3(5, -5, 5);
	testRenderer->trianglePos[2] = _Vector3(-5, -5, 5);// { _Vector3(0, 5, 5), _Vector3(5, -5, 5), _Vector3(-5, -5, 5) };
	
	//testRenderer->triangleColor ;
	testRenderer->triangleIndices[0] = 0;
	testRenderer->triangleIndices[1] = 2; 
	testRenderer->triangleIndices[2] = 1;

	testRenderer->triangleData.channelMask = MeshData::POSITION | MeshData::COLOR;
	testRenderer->triangleData.posList = testRenderer->trianglePos;
	testRenderer->triangleData.colorList = testRenderer->triangleColor;
	testRenderer->triangleData.indices = testRenderer->triangleIndices;
	testRenderer->triangleData.numVertices = 3;
	testRenderer->triangleData.numIndices = 3;

	UpdateMesh(testRenderer->queue, testRenderer->triangle, 0, 0, &testRenderer->triangleData);

	YU_LOCAL_PERSIST Vector3 squarePos[4] = { _Vector3(-10.f, -10.f, -5.f), _Vector3(-10.f, 10.f, -5.f), _Vector3(10.f, -10.f, -5.f), _Vector3(10.f, 10.f, -5.f) };
	YU_LOCAL_PERSIST Color squareColor[4] = {};
	YU_LOCAL_PERSIST u32 squareIndices[6] = { 0, 1, 2, 2, 1, 3};
	testRenderer->square = CreateMesh(testRenderer->queue, 4, 6, MeshData::POSITION | MeshData::COLOR);
	
	YU_LOCAL_PERSIST  MeshData squareData = {};
	squareData.channelMask = MeshData::POSITION | MeshData::COLOR;

	squareData.posList = squarePos;
	squareData.colorList = squareColor;
	squareData.indices = squareIndices;
	squareData.numVertices = 4;
	squareData.numIndices = 6;
	UpdateMesh(testRenderer->queue, testRenderer->square, 0, 0, &squareData);
	
#if defined YU_OS_WIN32
	VertexShaderAPIData vsData = LoadVSFromFile("data/shaders/flat_vs.hlsl");
	PixelShaderAPIData psData = LoadPSFromFile("data/shaders/flat_ps.hlsl");
#elif defined YU_OS_MAC
	VertexShaderAPIData vsData = LoadVSFromFile("data/shaders/flat_vs.glsl");
	PixelShaderAPIData psData = LoadPSFromFile("data/shaders/flat_ps.glsl");
#endif
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

	TextureDesc texDesc = {};
	texDesc.format = TEX_FORMAT_R8G8B8A8_UNORM;
	texDesc.width = 4;
	texDesc.height = 4;
	texDesc.mipLevels = 1;
	for (int i = 0; i < 16; i++)
	{
		testRenderer->texels[i] = _Color(0, 0xFF, 0, 0);
	}
	testRenderer->texData.texels = &testRenderer->texels;
	testRenderer->texData.texDataSize = TextureSize(texDesc.format, texDesc.width, texDesc.height, 1, 1);

	testRenderer->textureSlot.textures = CreateTexture(testRenderer->queue, texDesc, &testRenderer->texData);

	SamplerStateDesc samplerDesc = { SamplerStateDesc::FILTER_POINT, SamplerStateDesc::ADDRESS_CLAMP, SamplerStateDesc::ADDRESS_CLAMP };
	testRenderer->textureSlot.sampler = CreateSampler(testRenderer->queue, samplerDesc);

	InsertFence(testRenderer->queue, testRenderer->createResourceFence);

	
	Matrix4x4 viewProjMatrix = camData.PerspectiveMatrixGl() * camData.ViewMatrix();
	Matrix4x4 viewMatrix = camData.ViewMatrix();
	Vector4 viewPos[4];
	Vector4 projPos[4];
	for (int i = 0; i < 4; i++)
	{
		viewPos[i] = viewMatrix * _Vector4(squarePos[i], 1);
		projPos[i] = viewProjMatrix * _Vector4(squarePos[i], 1);
		projPos[i] /= projPos[i].w;
	}
	
	return item;
}

}