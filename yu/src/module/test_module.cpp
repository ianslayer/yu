
#include "../core/system.h"
#define IMPL_MODULE
#include "module.h"

#include "vm.h"

#define MAX_INPUT 256
#define MAX_KEYBOARD_STATE 256

using namespace yu;

struct KeyState
{
	yu::u32 pressCount;
	yu::u32 pressed;
	bool prevPressed;
	bool prevReleased;
};

inline bool KeyDown(u8 key, const KeyState* state)
{
	if(state[key].pressed && state[key].prevReleased)
		return true;
	return false;
}

inline bool KeyPressed(u8 key, const KeyState* state)
{
	if(state[key].pressed)
		return true;
	return false;
}

inline bool KeyUp(u8 key, const KeyState* state)
{
	if(!state[key].pressed && state[key].prevPressed)
		return true;
	return false;
}

inline bool KeyReleased(u8 key, const KeyState* state)
{
	if(!state[key].pressed)
		return true;
	return false;
}

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
	yu::u32 buttonState;
	float leftX;
	float leftY;

	float rightX;
	float rightY;
};

struct InputSource
{
	yu::EventQueue* eventQueue;
};

struct InputResult : public yu::CompleteData
{
	yu::InputEvent	frameEvent[MAX_INPUT];
	KeyState	keyboardState[MAX_KEYBOARD_STATE];
	MouseState	mouseState;
	JoyState	joyState[4];
	int			numEvent;
};

YU_INTERNAL void ProcessInput(const InputSource& source, InputResult& output)
{
//	int	threadIdx =	GetWorkerThreadIdx();
//	Log("thread id:%d\n", threadIdx);

//	yu::Log("lala\n");

	/*
	yu::Log("test:%d\n", 10);
	*/
	output.numEvent = 0;
	InputEvent event;

	for (int i = 0; i < MAX_KEYBOARD_STATE; i++)
	{
		output.keyboardState[i].pressCount = 0;
		output.keyboardState[i].prevPressed = false;
		output.keyboardState[i].prevReleased = false;
	}

	output.mouseState.leftButton.pressCount = 0;
	output.mouseState.rightButton.pressCount = 0;
	output.mouseState.dx = output.mouseState.dy = 0.f;

	while (yu::DequeueEvent(source.eventQueue, event) && output.numEvent < MAX_INPUT)
	{
		Time sysInitTime = yu::SysStartTime();
		Time eventTime;
		eventTime.time = event.timeStamp;

		output.frameEvent[output.numEvent++] = event;

		switch (event.type)
		{
		case InputEvent::UNKNOWN:
		break;
		case InputEvent::KILL_FOCUS:
		{
			yu::memset(&output, 0, sizeof(output));
		}
		break;
		case InputEvent::KEYBOARD:
		{
			//Log("event time: %f ", ConvertToMs(eventTime - sysInitTime) / 1000.f);
			if (event.keyboardEvent.type == InputEvent::KeyboardEvent::DOWN)
			{
				//yu::Log("%c key down\n", event.keyboardEvent.key);
				if(output.keyboardState[event.keyboardEvent.key].pressCount == 0)
				{
					output.keyboardState[event.keyboardEvent.key].prevReleased = true;
				}
				output.keyboardState[event.keyboardEvent.key].pressCount++;
				output.keyboardState[event.keyboardEvent.key].pressed = 1;
			}
			else if (event.keyboardEvent.type == InputEvent::KeyboardEvent::UP)
			{
				//yu::Log("%c key up\n", event.keyboardEvent.key);
				output.keyboardState[event.keyboardEvent.key].pressed = 0;
				output.keyboardState[event.keyboardEvent.key].prevPressed = true;
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
				//Log("right button up: %f, %f\n", event.mouseEvent.x, event.mouseEvent.y);
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
void InputWorkFunc(WorkHandle inputWork)
{
	WorkData inputWorkData = yu::GetWorkData(inputWork);
	const InputSource* source = (const InputSource*) inputWorkData.userData;
	InputResult* result = (InputResult*) inputWorkData.completeData;
	ProcessInput(*source, *result);
}


struct Material
{
	VertexShaderHandle	vs;
	PixelShaderHandle	ps;

	TextureSlot			texture[8];
	int					numTextures;
	
	PipelineHandle		pipeline;
};

struct WorldPosition
{
	int chunkX, chunkY, chunkZ;
	Vector3 relPos;
};

inline WorldPosition WorldOrigin()
{
	WorldPosition result = {};
	return result;
}

struct Entity
{
	WorldPosition	worldPosition;
	Quaternion		orientation;
	MeshHandle		mesh;
	Material		material;

	Matrix4x4		  transform;
	ConstBufferHandle transformConstBuffer;
};

struct Voxel
{
	Color  color;	
};

struct Block
{
	bool		leaf;
	union
	{
		Voxel	voxel[8];
		Block*	child[8];
	};
	Entity* entityList;
};

struct Chunk
{
	int x, y, z;
	Block* blocks;
};

inline Matrix4x4 ViewMatrix(Vector3 pos, Vector3 lookAt, Vector3 right)
{
	Vector3 down = cross(lookAt, right);

	Matrix4x4 result = Matrix4x4(right.x, right.y, right.z,0,
					 down.x, down.y, down.z, 0,
					 lookAt.x, lookAt.y, lookAt.z, 0,
					 0, 0, 0, 1) * Translate(-pos);
	return result;
}

inline Matrix4x4 PerspectiveMatrixDX(float upTan, float downTan, float leftTan, float rightTan, float n, float f)
{
	float r = n * rightTan;
	float l = -n * leftTan;

	float t = n * upTan;
	float b = -n * downTan;

	float width = r - l;
	float height = t - b;
	float depth = f - n;

	return Scale(_Vector3(1, -1, 1)) * Matrix4x4(2.f * n / width, 0.f, -(l + r) / width, 0.f,
		0.f, 2.f * n / height, -(t + b) / height, 0.f,
		0.f, 0.f, -n / depth, (n * f) / depth,
		0.f, 0.f, 1.f, 0.f);
}

inline Matrix4x4 PerspectiveMatrixDX(float halfTanX, float n, float f, float filmWidth, float filmHeight)
{
	float upTan = halfTanX * (filmHeight/  filmWidth);
	return PerspectiveMatrixDX(upTan, upTan, halfTanX, halfTanX, n, f);
}

inline Matrix4x4 PerspectiveMatrixGL(float upTan, float downTan, float leftTan, float rightTan, float n, float f)
{
	float r = n * rightTan;
	float l = -n * leftTan;

	float t = n * upTan;
	float b = -n * downTan;

	float width = r - l;
	float height = t - b;
	float depth = f - n;

	return Scale(_Vector3(1, -1, 1)) *
		Matrix4x4(2.f * n / width, 0.f, -(l + r) / width, 0.f,
		0.f, 2.f * n / height, -(t + b) / height, 0.f,
		0.f, 0.f, -(n + f) / depth, (2.f * n * f) / depth,
		0.f, 0.f, 1.f, 0.f);
}

inline Matrix4x4 PerspectiveMatrixGL(float halfTanX, float n, float f, float filmWidth, float filmHeight)
{
	float upTan = halfTanX * (filmHeight / filmWidth);
	return PerspectiveMatrixGL(upTan, upTan, halfTanX, halfTanX, n, f);
}

inline Matrix4x4 PerspectiveMatrix(float upTan, float downTan, float leftTan, float rightTan, float n, float f)
{
	Matrix4x4 result;

#if defined YU_GL
	result = PerspectiveMatrixGL(upTan, downTan, leftTan, rightTan, n, f);
#elif defined YU_DX11
	result = PerspectiveMatrixDX(upTan, downTan, leftTan, rightTan, n, f);
#endif

	return result;
}

inline Matrix4x4 PerspectiveMatrix(float halfTanX, float n, float f, float filmWidth, float filmHeight)
{
	Matrix4x4 result;

#if defined YU_GL
	result = PerspectiveMatrixGL(halfTanX, n, f, filmWidth, filmHeight);
#elif defined YU_DX11
	result = PerspectiveMatrixDX(halfTanX, n, f, filmWidth, filmHeight);
#endif
	return result;
}

struct Camera
{
	int chunkX, chunkY, chunkZ;
	
	Vector3 relPosition;
	Vector3 lookAt;
	Vector3 right;

	float upTan;
	float downTan;
	float leftTan;
	float rightTan;

	float n;
	float f;

	float filmWidth;
	float filmHeight;
	
	ConstBufferHandle	constBuffer;
	CameraConstant		cameraConst; //GPU side const buffer	
};

inline void UpdateCameraConstant(RenderQueue* queue, Camera* camera)
{
	camera->cameraConst.viewMatrix = ViewMatrix(camera->relPosition, camera->lookAt, camera->right);
	camera->cameraConst.projectionMatrix = PerspectiveMatrix(camera->upTan, camera->downTan, camera->leftTan, camera->rightTan, camera->n, camera->f);

	UpdateConstBuffer(queue, camera->constBuffer, 0, sizeof(camera->cameraConst), &camera->cameraConst);
			
}

inline void CreateDefaultCamera(RenderQueue* queue, Camera* camera)
{
	// view transform
	camera->relPosition = _Vector3(0, 0, 0);

	//right hand coordinate
	camera->lookAt = _Vector3(-1, 0, 0);//view space +z
	camera->right = _Vector3(0, 1, 0); //view space +x
	//Vector3 down;    //view space +y, can be derived from lookat ^ right

	camera->leftTan = camera->rightTan = tan(3.14f / 4.f);
	camera->upTan = camera->downTan = camera->leftTan * (720.f / 1280.f);

	//projection
	camera->n = 0.1f;
	camera->f = 3000.f;

	camera->filmWidth = 1280;
	camera->filmHeight = 720;

	camera->constBuffer = CreateConstBuffer(queue, sizeof(CameraConstant), &camera->cameraConst);

	UpdateCameraConstant(queue, camera);
	
}

inline void UpdateCameraConstBuffer(Camera* camera)
{
	
}

struct World
{
	Chunk* chunks;
	
	
	Entity	storedEntities[1024 * 16];	
	
};

MeshData CreateMeshData(int numVertices, int numIndices)
{
	size_t meshSize = (sizeof(Vector3) + sizeof(Vector2) + sizeof(Color) )* numVertices + sizeof(u32) * numIndices;
	void* meshMem = GetCurrentAllocator()->Alloc(meshSize);

	MeshData meshData;
	meshData.position = (Vector3*) meshMem;
	meshData.texcoord = (Vector2*) ((u8*)meshMem + sizeof(Vector3) * numVertices);
	meshData.color = (Color*) ((u8*)meshMem + sizeof(Vector3) * numVertices + sizeof(Vector2) * numVertices);
	meshData.indices = (u32*) ((u8*)meshMem + (sizeof(Vector3) + sizeof(Vector2) + sizeof(Color)) * numVertices);

	meshData.numVertices = numVertices;
	meshData.numIndices = numIndices;

	meshData.channelMask = MeshData::POSITION | MeshData::TEXCOORD | MeshData::COLOR;
	return meshData;
}

void DestroyMeshData( MeshData meshData)
{
	GetCurrentAllocator()->Free( meshData.position);
}

MeshData CreateCubeMeshData()
{
	MeshData cubeMeshData = CreateMeshData(24, 36);
		
	Vector3 cube[8];
	cube[0] = _Vector3(1, 1, 1);
	cube[1] = _Vector3(1, 1, -1);
	cube[2] = _Vector3(1, -1, 1);
	cube[3] = _Vector3(1, -1, -1);
	cube[4] = _Vector3(-1, 1, 1);
	cube[5] = _Vector3(-1, 1, -1);
	cube[6] = _Vector3(-1, -1, 1);
	cube[7] = _Vector3(-1, -1, -1);

	
	for(int i = 0 ; i < 24; i++)
	{
		cubeMeshData.color[i] = {0, 1, 0};
	}

	for(int i = 0; i < 24; i++)
	{
		cubeMeshData.texcoord[i] = _Vector2(0.5, 0.5);
	}
	
	//+x face
	cubeMeshData.position[0] = cube[0]; 
	cubeMeshData.position[1] = cube[2]; 
	cubeMeshData.position[2] = cube[1]; 
	cubeMeshData.position[3] = cube[3]; 

	//+y face
	cubeMeshData.position[4] = cube[4]; 
	cubeMeshData.position[5] = cube[0];
	cubeMeshData.position[6] = cube[5];
	cubeMeshData.position[7] = cube[1];


	//-x face
	cubeMeshData.position[8] = cube[6];
	cubeMeshData.position[9] = cube[4];
	cubeMeshData.position[10] = cube[7];
	cubeMeshData.position[11] = cube[5];

	//-y face
	cubeMeshData.position[12] = cube[2];
	cubeMeshData.position[13] = cube[6];
	cubeMeshData.position[14] = cube[3];
	cubeMeshData.position[15] = cube[7];

	//+z face
	cubeMeshData.position[16] = cube[4];
	cubeMeshData.position[17] = cube[6];
	cubeMeshData.position[18] = cube[0];
	cubeMeshData.position[19] = cube[2];

	//-z face
	cubeMeshData.position[20] = cube[1];
	cubeMeshData.position[21] = cube[3];
	cubeMeshData.position[22] = cube[5];
	cubeMeshData.position[23] = cube[7];
		
	u32 indices[36] = 
		{
			//+x
			0, 1, 2,
			2, 1, 3,

			//+y
			4, 5, 6,
			6, 5, 7,

			//-x
			8, 9, 10,
			10, 9, 11,

			//-y
			12, 13, 14,
			14, 13, 15,

			//+z
			16, 17, 18,
			18, 17, 19,

			//-z
			20, 21, 22,
			22, 21, 23
		};		

	for(int i = 0; i < 36; i++)
	{
		cubeMeshData.indices[i] = indices[i];
	}
	
	return cubeMeshData;
}

void CreateEntity(RenderQueue* renderQueue, Entity* result, MeshData* meshData)
{
	result->mesh = CreateMesh(renderQueue, meshData->numVertices, meshData->numIndices, meshData->channelMask);
	UpdateMesh(renderQueue, result->mesh, 0, 0, meshData);
	result->worldPosition = WorldOrigin();	

	result->transform = Identity4x4();
	result->transformConstBuffer = yu::CreateConstBuffer(renderQueue, sizeof(TransformConstant), &result->transform);
}

struct UpdateRenderData
{
	CompleteDataRef<InputResult> inputResult;	
	
	Camera				camera;
	
	float			cameraMoveSpeed;
	float			cameraTurnSpeed;

	TextureSlot		flatTextureSlot;

	Entity			cube;
	
	PipelineHandle flatRenderPipeline;

	VertexShaderHandle flatVs;
	PixelShaderHandle flatPs;

	FenceHandle		createResourceFence;
	bool initialized;

	float time;
};

VertexShaderHandle YU_INTERNAL ReloadVertexShader(RenderQueue* queue, const char* shaderPath, VertexShaderHandle oldShaderHandle)
{
	FreeVertexShader(queue, oldShaderHandle);
	
	DataBlob vsData = {};
	vsData = ReadDataBlob(shaderPath);
	
	VertexShaderHandle newShader = CreateVertexShader(queue, vsData);
	
	return newShader;
}

PixelShaderHandle YU_INTERNAL ReloadPixelShader(RenderQueue* queue, const char* shaderPath, PixelShaderHandle oldShaderHandle)
{
	FreePixelShader(queue, oldShaderHandle);

	DataBlob psData = {};
	psData = ReadDataBlob(shaderPath);

	PixelShaderHandle newShader = CreatePixelShader(queue, psData);

	return newShader;
}

PipelineHandle YU_INTERNAL ReloadPipeline(RenderQueue* queue, PipelineHandle oldPipeline, VertexShaderHandle newVS, PixelShaderHandle newPS)
{
	FreePipeline(queue, oldPipeline);

	PipelineData newPipelineData = {newVS, newPS};

	PipelineHandle newPipeline = CreatePipeline(queue, newPipelineData);

	return newPipeline;
}

void YU_INTERNAL UpdateRender(UpdateRenderData* data)
{
	yu::u64 frameCount = FrameCount();
		
	yu::RenderQueue* queue = yu::GetThreadLocalRenderQueue();
	
	if(!data->initialized)
	{
	   	CreateDefaultCamera(queue, &data->camera);
		
		data->camera.relPosition = _Vector3(50, 0, 0);
		

		DataBlob flatVsData = {};
		DataBlob flatPsData = {};
		char dataPath[1024];
#if defined YU_DX11
		flatVsData = ReadDataBlob(BuildDataPath("data/shaders/flat_vs.hlsl.cache", dataPath, 1024));
		flatPsData = ReadDataBlob(BuildDataPath("data/shaders/flat_ps.hlsl.cache", dataPath, 1024));
#elif defined YU_GL
		flatVsData = ReadDataBlob(BuildDataPath("data/shaders/flat_vs.glsl", dataPath, 1024));
		flatPsData = ReadDataBlob(BuildDataPath("data/shaders/flat_ps.glsl", dataPath, 1024));	
#endif		
		data->flatVs = CreateVertexShader(queue, flatVsData);
		data->flatPs = CreatePixelShader(queue, flatPsData);

		MeshData cubeMesh = CreateCubeMeshData();
		
	 	CreateEntity(queue, &data->cube, &cubeMesh);

		
		PipelineData flatPipelineData;
		flatPipelineData.vs = data->flatVs;
		flatPipelineData.ps = data->flatPs;

		data->flatRenderPipeline = CreatePipeline(queue, flatPipelineData);		
		
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

			data->flatTextureSlot.textures = CreateTexture(queue, texDesc, &texData);

			SamplerStateDesc samplerDesc = { SamplerStateDesc::FILTER_POINT, SamplerStateDesc::ADDRESS_CLAMP, SamplerStateDesc::ADDRESS_CLAMP };
			data->flatTextureSlot.sampler = CreateSampler(queue, samplerDesc);
		}
		

	   	data->createResourceFence = CreateFence(queue);
		InsertFence(queue, data->createResourceFence);
		WaitAndResetFence(queue, data->createResourceFence);

		FreeDataBlob(flatVsData);
		FreeDataBlob(flatPsData);
		DestroyMeshData( cubeMesh);

		data->time = 0.f;
		data->initialized = true;
	}
	
//	yu::Log("update render\n");

	float camTurnSpeed = 0.01f;
	float camMoveSpeed = 0.1f;
	
	Camera cameraData =  data->camera;
	Vector3 camLookAt = cameraData.lookAt;
	Vector3 camAheadDir = _Vector3(camLookAt.x, camLookAt.y, 0);
	Vector3 camRight = cameraData.right;
	Vector3 camPos = cameraData.relPosition;
	Vector3 camUp = -cross(camLookAt, camRight);

	if (data->inputResult->mouseState.captured || data->inputResult->mouseState.rightButton.pressed == 1)
	{
		Matrix3x3 yaw = Matrix3x3::RotateAxis(camUp, -data->inputResult->mouseState.dx * camTurnSpeed);
		cameraData.lookAt = yaw * camLookAt;
		cameraData.right = yaw * camRight;

		Matrix3x3 pitch = Matrix3x3::RotateAxis(camRight, -data->inputResult->mouseState.dy * camTurnSpeed);
		cameraData.lookAt = pitch * cameraData.lookAt;

		cameraData.right.z = 0;
		cameraData.right.Normalize();
		cameraData.lookAt = cross(cameraData.right, cross(cameraData.lookAt, cameraData.right));
		cameraData.lookAt.Normalize();
	}

	camLookAt = cameraData.lookAt;
	camRight = cameraData.right;

	KeyState* keyState = data->inputResult->keyboardState;
	
	if(KeyPressed('W', keyState))
	{
		camPos += camAheadDir * camMoveSpeed;
	}
	if(KeyPressed('S', keyState))
	{
		camPos -= camAheadDir * camMoveSpeed;
	}
	if(KeyPressed('D', keyState))
	{
		camPos += camRight * camMoveSpeed;
	}
	if(KeyPressed('A', keyState))
	{
		camPos -= camRight * camMoveSpeed; 
	}

	if(KeyDown('R', keyState))
	{
		//
		//yu::FilterLog(LOG_ERROR, "should reload shader\n");

		StackAllocator* scratchBuffer = CreateStackAllocator(1 * 1024 * 1024, GetCurrentAllocator());
		PushAllocator(scratchBuffer);
		
		char dataPath[1024];
		data->flatVs = ReloadVertexShader(queue, BuildDataPath("data/shaders/flat_vs.glsl", dataPath, 1024), data->flatVs);
		data->flatPs = ReloadPixelShader(queue, BuildDataPath("data/shaders/flat_ps.glsl", dataPath, 1024), data->flatPs);
		data->flatRenderPipeline = ReloadPipeline(queue, data->flatRenderPipeline, data->flatVs, data->flatPs);
		InsertFence(queue, data->createResourceFence);

		WaitAndResetFence(queue, data->createResourceFence);

		PopAllocator();
		FreeStackAllocator(scratchBuffer);
	}

	cameraData.relPosition = camPos;

	data->camera = cameraData;
//	data->camera.position = _Vector3(10, 0, 0);

	UpdateCameraConstant(queue, &data->camera);

	data->time += 0.1f;
	
	data->time = 2.f * 3.1414f * (int)((data->time) / (2.f * 3.1415f)) + (data->time - 2.f * 3.1414f * (int)((data->time) / (2.f * 3.1415f)));
	data->cube.transform = Translate(_Vector3(0, 0, sin(data->time))); 
	UpdateConstBuffer(queue, data->cube.transformConstBuffer, 0, sizeof(data->cube.transform), &data->cube.transform);	
	
	RenderResource flatRenderResource = {};
	flatRenderResource.numPsTexture = 1;
	flatRenderResource.psTextures = &data->flatTextureSlot;

	RenderTextureHandle frameBuffer = GetFrameBufferRenderTexture(GetRenderer());
	//BeginRenderPass(queue, frameBuffer);
	
	Render(queue, frameBuffer, data->camera.constBuffer, data->cube.mesh, data->cube.transformConstBuffer, data->flatRenderPipeline, flatRenderResource);

	//EndRenderPass(queue);
	Flush(queue);
	
	int workerThreadIndex = yu::GetWorkerThreadIndex();

#if defined YU_DEBUG
	int queueIndex = yu::GetRenderQueueIndex(queue);	
	yu::FilterLog(LOG_MESSAGE, "module frame:%d update ended, queue index: %d, worker index: %d\n", frameCount, queueIndex, workerThreadIndex);
#endif
}

void UpdateRenderFunc(WorkHandle updateRenderWork)
{
	UpdateRender((UpdateRenderData*)yu::GetWorkData(updateRenderWork).userData);
}

#define ON_APPEND(name) void name(struct AppendableField* field, YuCore* core, Module* module)
typedef ON_APPEND(OnAppendFunc);

struct AppendableField
{
	OnAppendFunc* OnAppend;
	bool initialized;
	AppendableField* nextField;
};

struct UpdateRenderWork: public AppendableField
{
	WorkHandle			updateRenderWorkItem;
	UpdateRenderData	updateRenderData;
};

struct InputWork : public AppendableField
{	
	WorkHandle 	inputWorkItem;
	InputSource inputSource;
	InputResult inputResult;
};

ON_APPEND(AppendInputWork)
{
	InputWork* inputWork = (InputWork*) field;
	if(!inputWork->initialized)
	{
		inputWork->inputWorkItem = yu::NewWorkItem();
		inputWork->inputSource.eventQueue = yu::CreateEventQueue(core->windowManager);
		inputWork->initialized = true;
	}
}

struct ModuleData
{
	ModuleData()
	{
		firstField = nullptr;
	}
	
	AppendableField* firstField;
 
	InputWork 			inputWork;
	UpdateRenderWork	updateRenderWork;
};


ON_APPEND(AppendUpdateRenderWork)
{
	UpdateRenderWork* updateRenderWork = (UpdateRenderWork*) field;
	ModuleData* moduleData = (ModuleData*) module->moduleData;
	if(!updateRenderWork->initialized)
	{
		updateRenderWork->updateRenderWorkItem = yu::NewWorkItem();
		updateRenderWork->updateRenderData.inputResult = &moduleData->inputWork.inputResult;
		updateRenderWork->initialized = true;
	}
}


#define ADD_FIELD(field, appendFunc)  yu::Log(#field); yu::Log(" appended\n");  moduleData->field.OnAppend = appendFunc; \
		if(!moduleData->field.initialized) { \
		moduleData->field.nextField = moduleData->firstField;	\
		moduleData->firstField = &moduleData->field; }

void AppendField(YuCore* core, Module* module, ModuleData* moduleData)
{
	ADD_FIELD(inputWork, AppendInputWork);
	ADD_FIELD(updateRenderWork, AppendUpdateRenderWork);
	
	for(AppendableField* field = moduleData->firstField; field ; field = field->nextField)
	{
		field->OnAppend(field, core, module);
	}

	moduleData->firstField = nullptr;
}

extern "C" MODULE_UPDATE(ModuleUpdate)
{
	ModuleData* moduleData = (ModuleData*) context->moduleData;
	gYuCore = core;

	yu::SetLogFilter(LOG_INFO);

	if(!context->initialized)
	{
		void* moduleScratchBuffer = core->sysAllocator->Alloc(sizeof(ModuleData) + 1024 * 1024);
		yu::memset(moduleScratchBuffer, 0, sizeof(ModuleData) + 1024 * 1024);
		context->moduleData = moduleData = new(moduleScratchBuffer) ModuleData();
		context->initialized = true;
	}
	
	if(context->reloaded)
	{
		yu::FilterLog(LOG_MESSAGE, "module: reload\n");
		AppendField(core, context, moduleData);
	}
		
	yu::SetWorkFunc(moduleData->inputWork.inputWorkItem, InputWorkFunc, nullptr);
	yu::SetWorkFunc(moduleData->updateRenderWork.updateRenderWorkItem, UpdateRenderFunc, nullptr);

	WorkData inputWorkData = {&moduleData->inputWork.inputSource, &moduleData->inputWork.inputResult};
	yu::SetWorkData(moduleData->inputWork.inputWorkItem, inputWorkData);
	moduleData->updateRenderWork.updateRenderData.inputResult = &moduleData->inputWork.inputResult;
	WorkData updateRenderData = {&moduleData->updateRenderWork.updateRenderData, nullptr};
	yu::SetWorkData(moduleData->updateRenderWork.updateRenderWorkItem, updateRenderData);
	
	yu::ResetWorkItem(moduleData->inputWork.inputWorkItem);
	yu::ResetWorkItem(moduleData->updateRenderWork.updateRenderWorkItem);
	
	yu::SubmitWorkItem(moduleData->inputWork.inputWorkItem, &core->startFrameLock, 1);
	
	yu::SubmitWorkItem(moduleData->updateRenderWork.updateRenderWorkItem, &moduleData->inputWork.inputWorkItem, 1);
	yu::AddEndFrameDependency(moduleData->updateRenderWork.updateRenderWorkItem);


}
