#include "../core/platform.h"
#include "../core/allocator.h"
#include "../core/thread.h"
#include "../core/worker.h"
#include "../core/log.h"
#include "../core/system.h"
#include "../core/timer.h"
#include "../core/file.h"
#include "../container/dequeue.h"
#include "../renderer/renderer.h"

#include "stargazer.h"
#include <new>

namespace yu
{

struct StarGazer
{
	//pre-defined work
	WorkItem* startWork;
	WorkItem* inputWork;
	WorkItem* shaderReloadWork;
	WorkItem* cameraControllerWork;
	WorkItem* renderWork;
	WorkItem* endWork;

	CameraHandle	camera;
	FenceHandle		controlFence;
};

StarGazer starGazerApp;
StarGazer* gStarGazer = &starGazerApp;

WorkItem* InputWorkItem(WindowManager* winMgr, Allocator* allocator);
WorkItem* TestRenderItem(Renderer* renderer, CameraHandle camera, Allocator* allocator);
WorkItem* CameraControlItem(WorkItem* inputWorkItem, CameraHandle camHandle, FenceHandle controlFence, Allocator* allocator);

void FreeWorkItem(WorkItem* item);

void Clear(StarGazer* starGazer)
{
	ResetWorkItem(starGazer->inputWork);
	ResetWorkItem(starGazer->cameraControllerWork);
	ResetWorkItem(starGazer->renderWork);
}

void AddEndFrameDependency(WorkItem* item);
	
void SubmitWork(StarGazer* starGazer)
{
	SubmitWorkItem(starGazer->inputWork, &starGazer->startWork, 1);
	SubmitWorkItem(starGazer->cameraControllerWork, &starGazer->inputWork, 1);
	SubmitWorkItem(starGazer->renderWork, &starGazer->cameraControllerWork, 1);

	AddEndFrameDependency(starGazer->renderWork);
}

void InitStarGazer(WindowManager* winMgr, Allocator* allocator, WorkItem* startFrameLock, WorkItem* endFrameLock)
{
	gStarGazer->startWork = startFrameLock;
	gStarGazer->endWork = endFrameLock;

	gStarGazer->inputWork = InputWorkItem(winMgr, allocator);

	gStarGazer->renderWork = TestRenderItem(GetRenderer(), gStarGazer->camera, allocator);
	gStarGazer->controlFence = CreateFence(GetThreadLocalRenderQueue());
	gStarGazer->cameraControllerWork = CameraControlItem(gStarGazer->inputWork, gStarGazer->camera, gStarGazer->controlFence, allocator);

}

void FreeStarGazer(Allocator* allocator)
{

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

struct InputSource
{
	EventQueue* eventQueue;
};

struct InputResult : public CompleteData
{
	InputEvent	frameEvent[MAX_INPUT];
	KeyState	keyboardState[MAX_KEYBOARD_STATE];
	MouseState	mouseState;
	JoyState	joyState[4];
	int			numEvent;
};

YU_INTERNAL void ProcessInput(const InputSource& source, InputResult& output)
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

	while (DequeueEvent(source.eventQueue, event) && output.numEvent < MAX_INPUT)
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
					output.mouseState.dy += (event.mouseEvent.y - output.mouseState.y);
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
			
			output.mouseState.x = event.mouseEvent.x;
			output.mouseState.y = event.mouseEvent.y;

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
	WorkData inputWorkData = GetWorkData(item);
	const InputSource* source = (const InputSource*) inputWorkData.userData;
	InputResult* result = (InputResult*) inputWorkData.completeData;
	ProcessInput(*source, *result);
}

WorkItem* InputWorkItem(WindowManager* winMgr,Allocator* allocator)
{
	WorkItem* item = NewWorkItem();
	SetWorkFunc(item, InputWorkFunc, nullptr);
	InputSource* source = New<InputSource>(allocator);
	source->eventQueue = CreateEventQueue(winMgr, allocator);
	InputResult* result = New<InputResult>(allocator);
	memset(result, 0, sizeof(*result));

	WorkData inputWorkData = {source, result};
	SetWorkData(item, inputWorkData);
	return item;
}

struct CameraControllerInput
{
	CompleteDataRef<InputResult> inputResult;
	CameraHandle camera;
	FenceHandle	controlFence;
	float		moveSpeed;
	float		turnSpeed;
};


struct CameraControllerOutput : public CompleteData
{
	CameraData		updatedCamera;
};

void CameraControl(const CameraControllerInput& input, CameraControllerOutput& output)
{
	//int	threadIdx = GetWorkerThreadIdx();
	//Log("thread id:%d\n", threadIdx);

	//float eyeHeight = GetHmdEyeHeight();

	RenderQueue* queue = GetThreadLocalRenderQueue();
	
	CameraData data = GetCameraData(queue, input.camera);

	Vector3 lookAt = data.lookAt;
	Vector3 aheadDir = _Vector3(lookAt.x, lookAt.y, 0);
	Vector3 right = data.right;
	Vector3 position = data.position;
	Vector3 up = -cross(lookAt, right);

	if (input.inputResult->mouseState.captured || input.inputResult->mouseState.rightButton.pressed == 1)
	{
		Matrix3x3 yaw = Matrix3x3::RotateAxis(up, -input.inputResult->mouseState.dx * input.turnSpeed);
		data.lookAt = yaw * lookAt;
		data.right = yaw * right;

		Matrix3x3 pitch = Matrix3x3::RotateAxis(right, -input.inputResult->mouseState.dy * input.turnSpeed);
		data.lookAt = pitch * data.lookAt;

		data.right.z = 0;
		data.right.Normalize();
		data.lookAt = cross(data.right, cross(data.lookAt, data.right));
		data.lookAt.Normalize();
	}

	lookAt = data.lookAt;
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
		data.lookAt = yaw * lookAt;
		data.right = yaw * right;

		Matrix3x3 pitch = Matrix3x3::RotateAxis(right, input.inputResult->joyState[0].rightY * input.turnSpeed * 2);
		data.lookAt = pitch * data.lookAt;

		data.right.z = 0;
		data.right.Normalize();
		data.lookAt = cross(data.right, cross(data.lookAt, data.right));
		data.lookAt.Normalize();

		position += aheadDir * input.moveSpeed * input.inputResult->joyState[0].leftY;
		position += right * input.moveSpeed * input.inputResult->joyState[0].leftX;
	 }

	data.position = position;
	UpdateCamera(queue, input.camera, data);

	InsertFence(queue, input.controlFence);
	WaitFence(queue, input.controlFence);
	ResetFence(queue, input.controlFence);
	
	output.updatedCamera = data;

}

void CameraControlWorkItem(WorkItem* item)
{
	WorkData cameraControlWorkData = GetWorkData(item);
	const CameraControllerInput* input = (const CameraControllerInput*)cameraControlWorkData.userData;
	CameraControllerOutput* output = (CameraControllerOutput*)cameraControlWorkData.completeData;
	CameraControl(*input, *output);
}

WorkItem* CameraControlItem(WorkItem* inputWorkItem, CameraHandle camHandle, FenceHandle controlFence, Allocator* allocator)
{
	WorkData inputWorkData = GetWorkData(inputWorkItem);
	InputResult* result = (InputResult*)inputWorkData.completeData;

	WorkItem* item =  NewWorkItem();

	CameraControllerInput* input = New<CameraControllerInput>(allocator);
	input->inputResult = result;
	input->camera = camHandle;
	input->controlFence = controlFence;
	input->moveSpeed = 0.1f;
	input->turnSpeed = 0.01f;
	CameraControllerOutput* output = New<CameraControllerOutput>(allocator);
	SetWorkFunc(item, CameraControlWorkItem, nullptr);

	WorkData CameraControlWorkData = {input, output};
	
	SetWorkData(item, CameraControlWorkData);
	
	return item;
}


struct TestRenderData : public CompleteData
{
	CameraHandle camera;
	CameraHandle vrCamera[2];

	CameraControllerOutput* updatedCam;

	MeshHandle	triangle;
	MeshHandle square;
	MeshHandle screenQuad;

	RenderResource::TextureSlot flatTextureSlot;
	RenderResource::TextureSlot	blitTextureSlot;

	PipelineHandle		flatRenderPipeline;
	PipelineHandle		blitPipeline;
	
	VertexShaderHandle	blitVs;
	VertexShaderHandle	flatVs;
	PixelShaderHandle	flatPs;

	//vr rendering
	TextureHandle										eyeTexture[2];
	RenderTextureHandle									eyeRenderTexture[2];

	TextureHandle										fractalTexture[2];
	RenderTextureHandle									fractalRenderTexture[2];

	TextureHandle										fractalVisTexture;
	RenderTextureHandle									fractalVisRenderTexture;

	FenceHandle											createResourceFence;
	FenceHandle											renderFence;
	
	bool initialized = false;

	Allocator* allocator;
};

struct ReloadData
{
	VertexShaderHandle	blitVs;
	VertexShaderHandle	flatVs;
	PixelShaderHandle	flatPs;
	
};

void Reload(ReloadData* reloadData)
{
	
}

	
void Render(TestRenderData* renderData)
{
	u64 frameCount = FrameCount();
	if(!renderData->initialized)
	{

		
		//RenderQueue* queue = GetRenderQueue(0);
		RenderQueue* queue = GetThreadLocalRenderQueue();

		CameraData camData = DefaultCamera();
		camData.position = _Vector3(50, 0, 0.f);

		UpdateCamera(queue, gStarGazer->camera, camData);
		
		renderData->createResourceFence = CreateFence(queue);
		renderData->renderFence = CreateFence(queue);
		
		renderData->triangle = CreateMesh(queue, 3, 3, MeshData::POSITION | MeshData::COLOR| MeshData::TEXCOORD);

		Vector3 trianglePos[3];
		trianglePos[0] = _Vector3(0, 5, 5);
		trianglePos[1] = _Vector3(5, -5, 5);
		trianglePos[2] = _Vector3(-5, -5, 5);// { _Vector3(0, 5, 5), _Vector3(5, -5, 5), _Vector3(-5, -5, 5) };
		
		Color triangleColor[3] = {};
		
		Vector2 triTexcoord[3] = { _Vector2(0.f, 1.f), _Vector2(0.f, 0.f), _Vector2(1.f, 1.f) };

		unsigned int triangleIndices[3] = {0, 2, 1};

		MeshData triangleData;
		triangleData.channelMask = MeshData::POSITION | MeshData::COLOR | MeshData::TEXCOORD;
		triangleData.posList = trianglePos;
		triangleData.colorList = triangleColor;
		triangleData.indices = triangleIndices;
		triangleData.texcoordList = triTexcoord;
		triangleData.numVertices = 3;
		triangleData.numIndices = 3;

		UpdateMesh(queue, renderData->triangle, 0, 0, &triangleData);
		

		Vector3 squarePos[4] = { _Vector3(-10.f, -10.f, -5.f), _Vector3(-10.f, 10.f, -5.f), _Vector3(10.f, -10.f, -5.f), _Vector3(10.f, 10.f, -5.f) };
		Color squareColor[4] = {};
		u32 squareIndices[6] = { 0, 1, 2, 2, 1, 3};
		Vector2 texcoord[4] = {_Vector2(0.f, 1.f), _Vector2(0.f, 0.f), _Vector2(1.f, 1.f), _Vector2(1.f, 0.f) };
		
		renderData->square = CreateMesh(queue, 4, 6, MeshData::POSITION | MeshData::COLOR | MeshData::TEXCOORD);
		
		MeshData squareData = {};
		squareData.channelMask = MeshData::POSITION | MeshData::COLOR | MeshData::TEXCOORD;

		squareData.posList = squarePos;
		squareData.texcoordList = texcoord;
		squareData.colorList = squareColor;
		squareData.indices = squareIndices;
		squareData.numVertices = 4;
		squareData.numIndices = 6;
		UpdateMesh(queue, renderData->square, 0, 0, &squareData);
		
		Vector3 screenQuadPos[4] = { _Vector3(-1.0f, -1.0f, 0.5f), _Vector3(-1.f, 1.f, 0.5f), _Vector3(1.f, -1.f, 0.5f), _Vector3(1.f, 1.f, 0.5f) };

		renderData->screenQuad = CreateMesh(queue, 4, 6, MeshData::POSITION | MeshData::COLOR | MeshData::TEXCOORD);
		MeshData screenQuadData = {};
		screenQuadData.channelMask = MeshData::POSITION | MeshData::COLOR | MeshData::TEXCOORD;

		screenQuadData.posList = screenQuadPos;
		screenQuadData.texcoordList = texcoord;
		screenQuadData.colorList = squareColor;
		screenQuadData.indices = squareIndices;
		screenQuadData.numVertices = 4;
		screenQuadData.numIndices = 6;
		UpdateMesh(queue, renderData->screenQuad, 0, 0, &screenQuadData);
	
		DataBlob blitVsData = {};
		DataBlob flatVsData = {};
		DataBlob flatPsData = {};

		char dataPath[1024];
	#if defined YU_DX11
		CreateShaderCache(BuildDataPath("data/shaders/blit_vs.hlsl", dataPath, 1024), "main", "vs_5_0");
		CreateShaderCache(BuildDataPath("data/shaders/flat_vs.hlsl", dataPath, 1024), "main", "vs_5_0");
		CreateShaderCache(BuildDataPath("data/shaders/flat_ps.hlsl", dataPath, 1024), "main", "ps_5_0");

		blitVsData = ReadDataBlob(BuildDataPath("data/shaders/blit_vs.hlsl.cache", dataPath, 1024), renderData->allocator);
		flatVsData = ReadDataBlob(BuildDataPath("data/shaders/flat_vs.hlsl.cache", dataPath, 1024), renderData->allocator);
		flatPsData = ReadDataBlob(BuildDataPath("data/shaders/flat_ps.hlsl.cache", dataPath, 1024), renderData->allocator);
	#elif defined YU_GL
		blitVsData = ReadDataBlob(BuildDataPath("data/shaders/blit_vs.glsl", dataPath, 1024), renderData->allocator);
		flatVsData = ReadDataBlob(BuildDataPath("data/shaders/flat_vs.glsl", dataPath, 1024), renderData->allocator);
		flatPsData = ReadDataBlob(BuildDataPath("data/shaders/flat_ps.glsl", dataPath, 1024), renderData->allocator);
	#endif
	
		renderData->flatVs = CreateVertexShader(queue, flatVsData);
		renderData->blitVs = CreateVertexShader(queue, blitVsData);
		renderData->flatPs = CreatePixelShader(queue, flatPsData);
		PipelineData flatPipelineData;
		flatPipelineData.vs = renderData->flatVs;
		flatPipelineData.ps = renderData->flatPs;
		PipelineData blitPipelineData;
		blitPipelineData.vs = renderData->blitVs;
		blitPipelineData.ps = renderData->flatPs;
		
		renderData->flatRenderPipeline = CreatePipeline(queue, flatPipelineData);
		renderData->blitPipeline = CreatePipeline(queue, blitPipelineData);

		Color texels[16];
		for (int i = 0; i < 16; i++)
		{
			texels[i] = _Color(0, 0, 0xFF, 0);
		}

		TextureMipData texData;
		{
			TextureDesc texDesc = {};
			texDesc.format = TEX_FORMAT_R8G8B8A8_UNORM;
			texDesc.width = 4;
			texDesc.height = 4;
			texDesc.mipLevels = 1;

			texData.texels = texels;
			texData.texDataSize = TextureSize(texDesc.format, texDesc.width, texDesc.height, 1, 1);

			renderData->flatTextureSlot.textures = CreateTexture(queue, texDesc, &texData);

			SamplerStateDesc samplerDesc = { SamplerStateDesc::FILTER_POINT, SamplerStateDesc::ADDRESS_CLAMP, SamplerStateDesc::ADDRESS_CLAMP };
			renderData->flatTextureSlot.sampler = CreateSampler(queue, samplerDesc);
		}

		{
			TextureDesc texDesc = {};
			texDesc.format = TEX_FORMAT_R8G8B8A8_UNORM;
			const RendererDesc& rendererDesc = GetRendererDesc(GetRenderer());
			texDesc.width = rendererDesc.width;
			texDesc.height = rendererDesc.height;
			texDesc.mipLevels = 1;
			texDesc.renderTexture = true;
			renderData->fractalVisTexture = CreateTexture(queue, texDesc);
			
			RenderTextureDesc rtDesc = {};
			rtDesc.refTexture = renderData->fractalVisTexture;
			rtDesc.mipLevel = 0;
			renderData->fractalVisRenderTexture = CreateRenderTexture(queue, rtDesc);
		}
		
		
		{
			TextureDesc texDesc = {};
			texDesc.format = TEX_FORMAT_R16G16_FLOAT;
			const RendererDesc& rendererDesc = GetRendererDesc(GetRenderer());
			texDesc.width = rendererDesc.width;
			texDesc.height = rendererDesc.height;
			texDesc.mipLevels = 1;
			texDesc.renderTexture = true;

			for(int tex = 0; tex < 2; tex++)
			{
				renderData->fractalTexture[tex] = CreateTexture(queue, texDesc);
				RenderTextureDesc rtDesc = {};
				rtDesc.mipLevel = 0;
				rtDesc.refTexture = renderData->fractalTexture[tex];
			
				renderData->fractalRenderTexture[tex] = CreateRenderTexture(queue, rtDesc);
			}
		}

		InsertFence(queue, renderData->createResourceFence);

		/*
		Matrix4x4 dxViewProjMatrix = camData.PerspectiveMatrixDx() * camData.ViewMatrix();
		Matrix4x4 glViewProjMatrix = camData.PerspectiveMatrixGl() * camData.ViewMatrix();
		Matrix4x4 viewMatrix = camData.ViewMatrix();
		Vector4 viewPos[4];
		Vector4 dxProjPos[4];
		Vector4 glProjPos[4];
		for (int i = 0; i < 4; i++)
		{
			viewPos[i] = viewMatrix * _Vector4(squarePos[i], 1);
			dxProjPos[i] = dxViewProjMatrix * _Vector4(squarePos[i], 1);
			dxProjPos[i] /= dxProjPos[i].w;

			glProjPos[i] = glViewProjMatrix * _Vector4(squarePos[i], 1);
			glProjPos[i] /= glProjPos[i].w;
		}
		*/
		WaitFence(queue, renderData->createResourceFence);

		CameraData testCamraData = GetCameraData(queue, gStarGazer->camera);
		
		FreeDataBlob(blitVsData, renderData->allocator);
		FreeDataBlob(flatVsData, renderData->allocator);
		FreeDataBlob(flatPsData, renderData->allocator);
		
		renderData->initialized = true;
	}
	Renderer* renderer = GetRenderer();
	//RenderQueue* queue = GetRenderQueue(0);
	RenderQueue* queue = GetThreadLocalRenderQueue();

	
	RenderResource flatRenderResource = {};
	flatRenderResource.numPsTexture = 1;
	flatRenderResource.psTextures = &renderData->flatTextureSlot;

	RenderResource	blitRenderResource = {};
	blitRenderResource.numPsTexture = 1;
	renderData->blitTextureSlot.textures = renderData->fractalVisTexture;
	renderData->blitTextureSlot.sampler = renderData->flatTextureSlot.sampler;
	blitRenderResource.psTextures = &renderData->blitTextureSlot;

	RenderTextureHandle frameBuffer = GetFrameBufferRenderTexture(renderer);
	
	if (GetRendererDesc(renderer).supportOvrRendering)
	{
		
		for (int eye = 0; eye < 2; eye++)
		{
			Vector2i eyeTextureSize = GetVRTextureSize(eye);
			TextureDesc texDesc = {};
			texDesc.format = TEX_FORMAT_R8G8B8A8_UNORM;
			texDesc.width = eyeTextureSize.x;
			texDesc.height = eyeTextureSize.y;
			texDesc.mipLevels = 1;
			texDesc.renderTexture = true;
			renderData->eyeTexture[eye] = CreateTexture(queue, texDesc, nullptr);
			RenderTextureDesc rtDesc = {};
			rtDesc.mipLevel = 0;
			rtDesc.refTexture = renderData->eyeTexture[eye];
			renderData->eyeRenderTexture[eye] = CreateRenderTexture(queue, rtDesc);
		}

		
	}
	
	Render(queue, frameBuffer, renderData->camera, renderData->triangle, renderData->flatRenderPipeline, flatRenderResource);
	
	//for (int i = 0; i < 1000; i++)
	Render(queue, frameBuffer, renderData->camera, renderData->square, renderData->flatRenderPipeline, flatRenderResource);

	//Render(queue, frameBuffer, renderData->camera, renderData->screenQuad, renderData->blitPipeline, blitRenderResource);

	Render(queue, frameBuffer, renderData->camera, renderData->screenQuad, renderData->flatRenderPipeline, flatRenderResource);

	Flush(queue);

#if defined YU_DEBUG
	int queueIndex = GetRenderQueueIndex(queue);
	int workerIndex = GetWorkerThreadIndex();
	
	FilterLog(LOG_MESSAGE, "stargazer frame:%d update ended, queue idx: %d , worker idx: %d\n", frameCount, queueIndex, workerIndex);
#endif
}

void TestRender(WorkItem* item)
{
	WorkData renderData = GetWorkData(item);
	TestRenderData* data = (TestRenderData*)renderData.completeData;
	Render(data);
}

void FreeTestRender(WorkItem* item)
{
	TestRenderData* data = (TestRenderData*)GetWorkData(item).completeData;
	Delete(data->allocator, data);
}

WorkItem* TestRenderItem(Renderer* renderer, CameraHandle camera, Allocator* allocator)
{
	WorkItem* item =  NewWorkItem();
	SetWorkFunc(item, TestRender, FreeTestRender);

	TestRenderData* data = New<TestRenderData>(allocator);
	data->allocator = allocator;

  	data->camera = gStarGazer->camera = CreateCamera(GetThreadLocalRenderQueue());
	
	WorkData renderWorkData = {0, data};
	
	SetWorkData(item, renderWorkData);
	
	data->camera = camera;
	
	return item;
}

}
