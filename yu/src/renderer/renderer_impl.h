#include "../core/platform.h"
#include "../core/allocator.h"
#include "../container/dequeue.h"
#include "../core/free_list.h"
#include "renderer.h"


#include "../../3rd_party/ovr/Src/OVR_CAPI.h"
#if defined YU_OS_WIN32
	#if defined YU_CPU_X86_64
		#pragma comment(lib, "../3rd_party/ovr/Lib/x64/VS2013/libovr64.lib")
	#elif defined YU_CPU_X86
		#pragma comment(lib, "../3rd_party/ovr/Lib/Win32/VS2013/libovr.lib")
	#endif
#pragma comment(lib, "ws2_32.lib") //what's this???
#endif

namespace yu
{

CameraData DefaultCamera()
{
	CameraData cam;
	// view transform
	cam.position = _Vector3(0, 0, 0);

	//right hand coordinate
	cam.lookAt = _Vector3(-1, 0, 0);//view space +z
	cam.right = _Vector3(0, 1, 0); //view space +x
	//Vector3 down;    //view space +y, can be derived from lookat ^ right

	cam.leftTan = cam.rightTan = tan(3.14f / 4.f);
	cam.upTan = cam.downTan = cam.leftTan * (720.f / 1280.f);

	//projection
	cam.n = 0.1f;
	cam.f = 3000.f;

	cam.filmWidth = 1280;
	cam.filmHeight = 720;

	return cam;
}

Matrix4x4 ViewMatrix(Vector3 pos, Vector3 lookAt, Vector3 right) 
{
	Vector3 down = cross(lookAt, right);
	return Matrix4x4(right.x, right.y, right.z,0,
					 down.x, down.y, down.z, 0,
					 lookAt.x, lookAt.y, lookAt.z, 0,
					 0, 0, 0, 1) * Translate(-pos);
}

Matrix4x4 PerspectiveMatrixDX(float upTan, float downTan, float leftTan, float rightTan, float n, float f)
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


Matrix4x4 PerspectiveMatrixDX(float halfTanX, float n, float f, float filmWidth, float filmHeight)
{
	float upTan = halfTanX * (filmHeight/  filmWidth);
	return PerspectiveMatrixDX(upTan, upTan, halfTanX, halfTanX, n, f);
}

Matrix4x4 PerspectiveMatrixGL(float upTan, float downTan, float leftTan, float rightTan, float n, float f)
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


Matrix4x4 PerspectiveMatrixGL(float halfTanX, float n, float f, float filmWidth, float filmHeight)
{
	float upTan = halfTanX * (filmHeight / filmWidth);
	return PerspectiveMatrixGL(upTan, upTan, halfTanX, halfTanX, n, f);
}

#define MAX_CAMERA 256
#define MAX_MESH 4096
#define MAX_PIPELINE 4096
#define MAX_SHADER 4096
#define MAX_TEXTURE 4096
#define	MAX_RENDER_TEXTURE 4096
#define MAX_SAMPLER 4096
#define MAX_FENCE 1024
#define MAX_RENDER_QUEUE 32

struct Fence
{
	std::atomic<bool>	cpuExecuted;
};

struct RenderThreadCmd
{
	enum CommandType
	{
		RESIZE_BUFFER,

		STOP_RENDER_THREAD,

		START_VR_MODE,
		STOP_VR_MODE,

		CREATE_MESH,
		//	CREATE_CAMERA,

		UPDATE_MESH,
		UPDATE_CAMERA,

		RENDER,
		SWAP,

		CREATE_VERTEX_SHADER,
		CREATE_PIXEL_SHADER,
		CREATE_PIPELINE,
		
#if defined YU_DEBUG || defined YU_TOOL
		RELOAD_VERTEX_SHADER,
		RELOAD_PIXEL_SHADER,
		RELOAD_PIPELINE,
#endif

		CREATE_TEXTURE,
		CREATE_RENDER_TEXTURE,
		CREATE_SAMPLER,

		CREATE_FENCE,
		INSERT_FENCE,
	};

	CommandType type;

	struct ResizeBufferCmd
	{
		unsigned int	width;
		unsigned int	height;
		TextureFormat		fmt;
	};

	struct CreateMeshCmd
	{
		MeshHandle	mesh;
		u32			numVertices;
		u32			numIndices;
		u32			vertChannelMask;
		MeshData*	meshData;
	};
	
	//TODO: merge the below two update into render cmd
	struct UpdateMeshCmd
	{
		MeshHandle	mesh;
		u32			startVertex;
		u32			startIndex;
		MeshData*	meshData;
	};

	struct UpdateCameraCmd
	{
		CameraHandle camera;
		CameraData	 camData;
	};

	struct CreateVertexShaderCmd
	{
		VertexShaderHandle	vertexShader;
		DataBlob			data;
	};

	struct CreatePixelShaderCmd
	{
		PixelShaderHandle	pixelShader;
		DataBlob			data;
	};

	struct CreatePipelineCmd
	{
		PipelineHandle	pipeline;
		PipelineData	data;
	};
	
#if defined (YU_DEBUG) || defined (YU_TOOL)
	struct ReloadPipelineCmd
	{
		PipelineHandle pipeline;
	};
#endif

	struct CreateTextureCmd
	{
		TextureHandle	texture;
		TextureDesc		desc;
		TextureMipData* initData;
	};

	struct CreateRenderTextureCmd
	{
		RenderTextureHandle	renderTexture;
		RenderTextureDesc	desc;
	};

	struct CreateSamplerCmd
	{
		SamplerHandle		sampler;
		SamplerStateDesc	desc;
	};

	struct CreateFenceCmd
	{
		FenceHandle			fence;
		bool				createGpuFence;
	};

	struct InsertFenceCmd
	{
		FenceHandle			fence;
	};

	struct RenderCmd
	{
		int renderListIndex;
	};

	struct SwapCmd
	{
		bool vsync;
	};

	union
	{
		ResizeBufferCmd			resizeBuffCmd;
		CreateMeshCmd			createMeshCmd;
		UpdateCameraCmd			updateCameraCmd;
		UpdateMeshCmd			updateMeshCmd;
		RenderCmd				renderCmd;
		SwapCmd					swapCmd;

		CreateVertexShaderCmd	createVSCmd;
		CreatePixelShaderCmd	createPSCmd;
		CreatePipelineCmd		createPipelineCmd;
		CreateTextureCmd		createTextureCmd;
		CreateRenderTextureCmd	createRenderTextureCmd;
		CreateSamplerCmd		createSamplerCmd;
		
		CreateFenceCmd			createFenceCmd;
		InsertFenceCmd			insertFenceCmd;
	};
};

struct RenderCmd
{
	CameraHandle	cam;
	RenderTextureHandle	renderTexture;
	MeshHandle		mesh;
	PipelineHandle	pipeline;
	RenderResource	resources;
	u32				startIndex = 0;
	u32				numIndex = 0;
};

#define MAX_RENDER_CMD 256
struct RenderCmdList
{
	RenderCmdList(Allocator* allocator) : cmdCount(0), renderInProgress(false), scratchBuffer(1024*1024, allocator) {}
	RenderCmd			cmd[MAX_RENDER_CMD];
	int					cmdCount;
	std::atomic<bool>	renderInProgress;
	StackAllocator		scratchBuffer;
};

#define MAX_RENDER_LIST 2
struct RenderQueue
{
	RenderQueue(Allocator* allocator)
	{
		renderList = DeepNewArray<RenderCmdList>(allocator, MAX_RENDER_LIST);
	}
	
	Renderer* renderer;
	SpscFifo<RenderThreadCmd, 256> cmdList;

	RenderCmdList*			renderList;
};


struct BaseDoubleBufferData
{
	BaseDoubleBufferData* nextDirty;
	YU_GLOBAL BaseDoubleBufferData* dirtyLink;

	u32 updateCount = 0;
	bool dirty;

	YU_CLASS_FUNCTION void SwapDirty()
	{
		for (BaseDoubleBufferData* data = dirtyLink; data != nullptr; data = data->nextDirty)
		{
			if (data->dirty)
			{
				data->updateCount++;
				data->dirty = false;
			}
		}

		dirtyLink = nullptr;
	}
};

template<class T>
struct DoubleBufferData : public BaseDoubleBufferData
{
	void InitData(T& _data)
	{
		data[0] = data[1] = _data;
		dirty = false;
		nextDirty = nullptr;
	}

	const T& GetConst() const
	{
		return data[updateCount & 1];
	}

	T& GetMutable()
	{
		return data[1 - updateCount & 1];
	}

	void UpdateData(const T& _data)
	{
		GetMutable() = _data;

		if (!dirty)
		{
			dirty = true;
			nextDirty = dirtyLink;
			dirtyLink = this;
		}
	}

	T	data[2];

};

struct DoubleBufferCameraData : public DoubleBufferData < CameraData >
{

};

BaseDoubleBufferData* BaseDoubleBufferData::dirtyLink;

YU_PRE_ALIGN(16)
struct CameraConstant
{
	Matrix4x4 viewMatrix;
	Matrix4x4 projectionMatrix;
} YU_POST_ALIGN(16);

struct MeshRenderData
{
	u32			numVertices = 0;
	u32			numIndices = 0;
	u32			channelMask = 0;
};

struct Renderer
{
	Renderer(Allocator* allocator)
	{
		renderQueue = DeepNewArray<RenderQueue>(allocator, MAX_RENDER_QUEUE);
	}
	
	IndexFreeList<MAX_CAMERA>							cameraIdList;
	DoubleBufferCameraData								cameraDataList[MAX_CAMERA];

	IndexFreeList<MAX_MESH>								meshIdList;
	MeshRenderData										meshList[MAX_MESH];
	
	IndexFreeList<MAX_SHADER>							vertexShaderIdList;
	IndexFreeList<MAX_SHADER>							pixelShaderIdList;

	IndexFreeList<MAX_PIPELINE>							pipelineIdList;
	PipelineData										pipelineDataList[MAX_PIPELINE];

	IndexFreeList<MAX_TEXTURE>							textureIdList;
	TextureDesc											textureDescList[MAX_TEXTURE];
	
	IndexFreeList<MAX_RENDER_TEXTURE>					renderTextureIdList;
	RenderTextureDesc									renderTextureDescList[MAX_RENDER_TEXTURE];

	IndexFreeList<MAX_SAMPLER>							samplerIdList;
	SamplerStateDesc									samplerDescList[MAX_SAMPLER];

	IndexFreeList<MAX_FENCE>							fenceIdList;
	Fence												fenceList[MAX_FENCE];
	
	RenderQueue*										renderQueue;
	std::atomic<int>									numQueue;

	RenderTextureHandle									frameBuffer;

	RendererDesc										rendererDesc;


	bool												vrRendering = false;
	RenderTextureHandle									eyeRenderTexture[2];
};

RenderTextureHandle	GetFrameBufferRenderTexture(Renderer* renderer)
{
	return renderer->frameBuffer;
}

const RendererDesc& GetRendererDesc(Renderer* renderer)
{
	return renderer->rendererDesc;
}

RenderQueue* CreateRenderQueue(Renderer* renderer)
{
	/*
	int queueIdx = renderer->renderQueueList.Alloc();
	RenderQueue* queue = renderer->renderQueueList.Get(queueIdx);
	renderer->renderQueue[renderer->numQueue] = queue;
	renderer->numQueue++;
	*/

	int queueIdx = renderer->numQueue.fetch_add(1);
	RenderQueue* queue = &renderer->renderQueue[queueIdx];

	queue->renderer = renderer;
	return queue;
}

YU_INTERNAL void BlockEnqueueCmd(RenderQueue* queue, RenderThreadCmd cmd)
{
	while (!queue->cmdList.Enqueue(cmd))
		;
}

YU_INTERNAL bool HasPos(u32 mask)
{
	return (mask & MeshData::POSITION) != 0;
}

YU_INTERNAL bool HasTexcoord(u32 mask)
{
	return (mask & MeshData::TEXCOORD) != 0;
}

YU_INTERNAL bool HasColor(u32 mask)
{
	return (mask & MeshData::COLOR) != 0;
}

YU_INTERNAL u32 VertexSize(u32 mask)
{
	u32 vertexSize = 0;
	if (HasPos(mask))
	{
		vertexSize += sizeof(Vector3);
	}
	if (HasTexcoord(mask))
	{
		vertexSize += sizeof(Vector2);
	}
	if (HasColor(mask))
	{
		vertexSize += sizeof(Color);
	}

	return vertexSize;
}

YU_INTERNAL void InterleaveVertexBuffer(void* vertexBuffer, MeshData* meshData, u32 channelMask, u32 startVertex, u32 numVertices)
{
	u8* vertex = (u8*) vertexBuffer;
	for (u32 i = startVertex; i < startVertex + numVertices; i++)
	{
		if (HasPos(channelMask) && HasPos(meshData->channelMask) )
		{
			*((Vector3*)vertex) = meshData->posList[i];
				vertex += sizeof(Vector3);
		}
		if (HasTexcoord(channelMask) && HasTexcoord(meshData->channelMask))
		{
			*((Vector2*)vertex) = meshData->texcoordList[i];
			vertex += sizeof(Vector2);
		}
		if (HasColor(channelMask) && HasColor(meshData->channelMask))
		{
			*((Color*)vertex) = meshData->colorList[i];
			vertex += sizeof(Color);
		}
	}
	
}

void StopRenderThread(RenderQueue* queue)
{
	//gRenderThreadRunning = false;
	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::STOP_RENDER_THREAD;
	BlockEnqueueCmd(queue, cmd);
}

struct OvrDevice
{
	ovrHmd				hmd = nullptr;                  // The handle of the headset
	bool				hmdDetected = false;
	ovrBool				initialized = false;
	ovrEyeRenderDesc	eyeRenderDesc[2];
};

YU_GLOBAL OvrDevice* gOvrDevice;

void StartVRRendering(RenderQueue* queue)
{
	Renderer* renderer = queue->renderer;
	if (renderer->rendererDesc.supportOvrRendering)
	{
		RenderThreadCmd cmd;
		cmd.type = RenderThreadCmd::START_VR_MODE;
		BlockEnqueueCmd(queue, cmd);
	}
}

void EndVRRendering(RenderQueue* queue)
{
	Renderer* renderer = queue->renderer;
	if (renderer->rendererDesc.supportOvrRendering)
	{
		RenderThreadCmd cmd;
		cmd.type = RenderThreadCmd::STOP_VR_MODE;
		BlockEnqueueCmd(queue, cmd);
	}
}

float GetHmdEyeHeight()
{
	float eyeHeight = 0.f;
	ovrPosef eyeRenderPose[2];
	if (gOvrDevice && gOvrDevice->hmd)
	{
#if defined YU_OS_WIN32
		eyeHeight = ovrHmd_GetFloat(gOvrDevice->hmd, OVR_KEY_EYE_HEIGHT, eyeHeight);

		ovrVector3f hmdToEyeViewOffset[2] = { gOvrDevice->eyeRenderDesc[0].HmdToEyeViewOffset,
			gOvrDevice->eyeRenderDesc[0].HmdToEyeViewOffset };

		ovrHmd_GetEyePoses(gOvrDevice->hmd, 0, hmdToEyeViewOffset, eyeRenderPose, NULL);
#endif
	}
	return eyeHeight;
}

Vector2i GetVRTextureSize(int eye)
{
	Vector2i size;
#if defined YU_OS_WIN32
	ovrSizei idealSize = ovrHmd_GetFovTextureSize(gOvrDevice->hmd, (ovrEyeType)eye, gOvrDevice->hmd->DefaultEyeFov[eye], 1.0f);
	size.w = idealSize.w;
	size.h = idealSize.h;
#endif
	return size;
}


MeshHandle	CreateMesh(RenderQueue* queue, u32 numVertices, u32 numIndices, u32 vertChannelMask)
{
	MeshHandle mesh;
	mesh.id = queue->renderer->meshIdList.Alloc();
	MeshRenderData* renderData = queue->renderer->meshList + mesh.id;
	renderData->channelMask = vertChannelMask;
	renderData->numVertices = numVertices;
	renderData->numIndices = numIndices;

	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::CREATE_MESH;
	cmd.createMeshCmd.mesh = mesh;
	cmd.createMeshCmd.meshData = nullptr;
	cmd.createMeshCmd.numVertices = numVertices;
	cmd.createMeshCmd.numIndices = numIndices;
	cmd.createMeshCmd.vertChannelMask = vertChannelMask;
	BlockEnqueueCmd(queue, cmd);

	return mesh;
}

void FreeMesh(RenderQueue* queue, MeshHandle mesh) //TODO : cleanup render thread data
{
	queue->renderer->meshIdList.DeferredFree(mesh.id);
}

/*
YU_INTERNAL MeshData* CopyMesh(MeshData* originMesh, Allocator* allocator)
{

}
*/

void UpdateMesh(RenderQueue* queue, MeshHandle mesh, 
	u32 startVert, u32 startIndex, 
	MeshData* inputSubMesh)
{

	Renderer* renderer = queue->renderer;

	MeshRenderData* meshRenderData = renderer->meshList + mesh.id;
	if (meshRenderData->channelMask != inputSubMesh->channelMask)
	{
		Log("error: UpdateMesh trying to update mesh in different vertex layout\n");
		return;
	}
	if (startVert + inputSubMesh->numVertices > meshRenderData->numVertices)
	{
		Log("error: UpdateMesh vertex number out of bound\n ");
		return;
	}
	if (startIndex + inputSubMesh->numIndices > meshRenderData->numIndices)
	{
		Log("error: UpdateMesh index number out of bound\n");
		return;
	}

	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::UPDATE_MESH;
	cmd.updateMeshCmd.mesh = mesh;
	cmd.updateMeshCmd.startVertex = startVert;
	cmd.updateMeshCmd.startIndex = startIndex;
	cmd.updateMeshCmd.meshData = inputSubMesh;
	BlockEnqueueCmd(queue, cmd);
}

VertexShaderHandle CreateVertexShader(RenderQueue* queue, const DataBlob& data)
{
	VertexShaderHandle vertexShader;
	vertexShader.id = queue->renderer->vertexShaderIdList.Alloc();

	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::CREATE_VERTEX_SHADER;
	cmd.createVSCmd.vertexShader = vertexShader;
	cmd.createVSCmd.data = data;

	BlockEnqueueCmd(queue, cmd);

	return vertexShader;
}


PixelShaderHandle CreatePixelShader(RenderQueue* queue, const DataBlob& data)
{
	PixelShaderHandle pixelShader;
	pixelShader.id = queue->renderer->pixelShaderIdList.Alloc();

	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::CREATE_PIXEL_SHADER;
	cmd.createPSCmd.pixelShader = pixelShader;
	cmd.createPSCmd.data = data;

	BlockEnqueueCmd(queue, cmd);

	return pixelShader;
}

PipelineHandle CreatePipeline(RenderQueue* queue, const PipelineData& data)
{
	PipelineHandle pipeline;
	pipeline.id = queue->renderer->pipelineIdList.Alloc();
	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::CREATE_PIPELINE;
	cmd.createPipelineCmd.pipeline = pipeline;
	cmd.createPipelineCmd.data = data;

	BlockEnqueueCmd(queue, cmd);

	return pipeline;
}

#if defined (YU_DEBUG) || defined (YU_TOOL)

void ReloadVertexShader(RenderQueue* queue, VertexShaderHandle vertexShader, const DataBlob& data)
{
	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::RELOAD_VERTEX_SHADER;
	cmd.createVSCmd.vertexShader = vertexShader;
	cmd.createVSCmd.data = data;

	BlockEnqueueCmd(queue, cmd);
}

void ReloadPixelShader(RenderQueue* queue, PixelShaderHandle pixelShader, const DataBlob& data)
{
	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::RELOAD_PIXEL_SHADER;
	cmd.createPSCmd.pixelShader = pixelShader;
	cmd.createPSCmd.data = data;

	BlockEnqueueCmd(queue, cmd);
}

void ReloadPipeline(RenderQueue* queue, PipelineHandle pipeline, const PipelineData& pipelineData)
{
	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::RELOAD_PIPELINE;
	cmd.createPipelineCmd.pipeline = pipeline;
	cmd.createPipelineCmd.data = pipelineData;
}
#endif

int TexelSize(TextureFormat format)
{
	switch (format)
	{
	case TEX_FORMAT_R8G8B8A8_UNORM:
	case TEX_FORMAT_R8G8B8A8_UNORM_SRGB:
		return 4;

	case TEX_FORMAT_R16G16_FLOAT:
		return 4;

	case TEX_FORMAT_UNKNOWN:
	case NUM_TEX_FORMATS:
	default:
	;
	}
	Log("error: unknown texture format\n");
	assert(0);
	return 0;
}

size_t BlockSize(TextureFormat format)
{


	assert(0);
	return 0;
}

int IsCompressedFormat(TextureFormat format)
{

	return false;
}

int TotalMipLevels(int width, int height)
{
	int totalLevelsX = 1;
	int totalLevelsY = 1;

	while (width > 1)
	{
		width /= 2;
		totalLevelsX++;
	}
	while (height > 1)
	{
		height /= 2;
		totalLevelsY++;
	}
	return max(totalLevelsX, totalLevelsY);
}

size_t TextureLevelSize(TextureFormat format, int width, int height, int depth, int mipSlice)
{
	int mipWidth = max(1, width >> mipSlice);
	int mipHeight = max(1, height >> mipSlice);
	int mipDepth = max(1, depth >> mipSlice);

	if (IsCompressedFormat(format))
	{
		return ((mipWidth + 3) / 4) * ((mipHeight + 3) / 4) * ((mipDepth + 3) / 4) * BlockSize(format);
	}

	return (mipWidth * mipHeight * mipDepth * TexelSize(format) + 3) & -4;
}

size_t TextureSize(TextureFormat format, int width, int height, int depth, int mipLevels)
{
	size_t size = 0;
	for (int i = 0; i < mipLevels; i++)
	{
		size += TextureLevelSize(format, width, height, depth, i);
	}

	return size;
}

//TODO: copy init data to scratch buffer so front end can manage TextureMipData life time
TextureHandle CreateTexture(RenderQueue* queue, const TextureDesc& desc, TextureMipData* initData)
{
	TextureHandle texture;
	texture.id = queue->renderer->textureIdList.Alloc();
	TextureDesc& texDesc = queue->renderer->textureDescList[texture.id];
	texDesc = desc;

	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::CREATE_TEXTURE;
	cmd.createTextureCmd.texture = texture;
	cmd.createTextureCmd.desc = desc;
	cmd.createTextureCmd.initData = initData;
	BlockEnqueueCmd(queue, cmd);

	return texture;
}

RenderTextureHandle CreateRenderTexture(RenderQueue* queue, const RenderTextureDesc& desc)
{
	RenderTextureHandle renderTexture;
	renderTexture.id = queue->renderer->renderTextureIdList.Alloc();
	RenderTextureDesc& rtDesc = queue->renderer->renderTextureDescList[renderTexture.id];
	rtDesc = desc;

	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::CREATE_RENDER_TEXTURE;
	cmd.createRenderTextureCmd.renderTexture = renderTexture;
	cmd.createRenderTextureCmd.desc = desc;
	BlockEnqueueCmd(queue, cmd);
	return renderTexture;
}

SamplerHandle CreateSampler(RenderQueue* queue, const SamplerStateDesc& desc)
{
	SamplerHandle sampler;
	sampler.id = queue->renderer->samplerIdList.Alloc();
	SamplerStateDesc& samplerDesc = queue->renderer->samplerDescList[sampler.id];
	samplerDesc = desc;

	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::CREATE_SAMPLER;
	cmd.createSamplerCmd.sampler = sampler;
	cmd.createSamplerCmd.desc = desc;
	BlockEnqueueCmd(queue, cmd);

	return sampler;
}

FenceHandle CreateFence(RenderQueue* queue)
{
	FenceHandle fence;
	fence.id = queue->renderer->fenceIdList.Alloc();
	queue->renderer->fenceList[fence.id].cpuExecuted = false;
	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::CREATE_FENCE;
	cmd.createFenceCmd.fence = fence;
	cmd.createFenceCmd.createGpuFence = false;
	BlockEnqueueCmd(queue, cmd);

	return fence;
}

void InsertFence(RenderQueue* queue, FenceHandle fence)
{
	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::INSERT_FENCE;
	cmd.insertFenceCmd.fence = fence;
	Flush(queue); //important: must ensure all previous commands are enqueued
	BlockEnqueueCmd(queue, cmd);
}

bool IsCPUComplete(RenderQueue* queue, FenceHandle fenceHandle)
{
	Fence* fence = queue->renderer->fenceList + fenceHandle.id;
	return fence->cpuExecuted;
}

void WaitFence(RenderQueue* queue, FenceHandle fence) //TODO: consider proper wait
{
	while (!IsCPUComplete(queue, fence))
		;
}

void Reset(RenderQueue* queue, FenceHandle fence)
{
	assert(queue->renderer->fenceList[fence.id].cpuExecuted == true);
	queue->renderer->fenceList[fence.id].cpuExecuted = false;
}

CameraHandle CreateCamera(RenderQueue* queue)
{
	CameraHandle camera;
	camera.id = queue->renderer->cameraIdList.Alloc();
	DoubleBufferCameraData* camData = queue->renderer->cameraDataList + camera.id;
	CameraData defaultCamera = DefaultCamera();
	camData->InitData(defaultCamera);
	UpdateCamera(queue, camera, defaultCamera);
	return camera;
}

CameraData GetCameraData(RenderQueue* queue, CameraHandle handle)
{
	return (queue->renderer->cameraDataList + handle.id)->GetConst();
}

void UpdateCamera(RenderQueue* queue, CameraHandle camera, const CameraData& cameraData)
{
	Renderer* renderer = queue->renderer;

	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::UPDATE_CAMERA;
	cmd.updateCameraCmd.camera = camera;
	cmd.updateCameraCmd.camData = cameraData;
	BlockEnqueueCmd(queue, cmd);
}

void Render(RenderQueue* queue, RenderTextureHandle renderTexture, CameraHandle cam, MeshHandle mesh, PipelineHandle pipeline, const RenderResource& resources)
{
	RenderCmdList* list;
	int listIndex;

	size_t resourceSize = 0;
	{
		resourceSize = sizeof(RenderResource::TextureSlot) * resources.numPsTexture;
	}
	
	for (;;)
	{
		for (int i = 0; i < MAX_RENDER_LIST; i++)
		{
			if (!queue->renderList[i].renderInProgress.load(std::memory_order_acquire) 
				&& resourceSize <= queue->renderList[i].scratchBuffer.Available() )
			{
				list = &queue->renderList[i];
				listIndex = i;
				goto ListFound;
			}
		}
	}
ListFound:
	
	MeshRenderData* meshRenderData = queue->renderer->meshList + mesh.id;
	RenderCmd& cmd = list->cmd[list->cmdCount];
	cmd.renderTexture = renderTexture;
	cmd.cam = cam;
	cmd.mesh = mesh;
	cmd.pipeline = pipeline;
	cmd.startIndex = 0;
	cmd.numIndex = meshRenderData->numIndices;

	{
		cmd.resources.numPsTexture = resources.numPsTexture;
		cmd.resources.psTextures = (RenderResource::TextureSlot*)list->scratchBuffer.Alloc(sizeof(RenderResource::TextureSlot) * resources.numPsTexture);
		memcpy(cmd.resources.psTextures, resources.psTextures, sizeof(RenderResource::TextureSlot) * resources.numPsTexture);
	}

	list->cmdCount++;
	
	if (list->cmdCount == MAX_RENDER_CMD)
	{
		RenderThreadCmd cmd;
		cmd.type = RenderThreadCmd::RENDER;
		cmd.renderCmd.renderListIndex = listIndex;
		queue->renderList[listIndex].renderInProgress.store(true, std::memory_order_release);
		BlockEnqueueCmd(queue, cmd);
	}
}

void Flush(RenderQueue* queue)
{
	for (int i = 0; i < MAX_RENDER_LIST; i++)
	{
		if (!queue->renderList[i].renderInProgress.load(std::memory_order_acquire) && queue->renderList[i].cmdCount > 0)
		{
			RenderThreadCmd cmd;
			cmd.type = RenderThreadCmd::RENDER;
			cmd.renderCmd.renderListIndex = i;
			queue->renderList[i].renderInProgress.store(true, std::memory_order_release);
			BlockEnqueueCmd(queue, cmd);
		}
	}
}

void Swap(RenderQueue* queue, bool vsync)
{
	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::SWAP;
	cmd.swapCmd.vsync = vsync;
	BlockEnqueueCmd(queue, cmd);
}

void CopyMesh(Renderer* renderer, MeshHandle handle, MeshData* outData, u32 channel);

}



