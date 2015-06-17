#include "../core/platform.h"
#include "../core/allocator.h"
#include "../container/queue.h"
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

#define MAX_MESH 4096
#define MAX_CONST_BUFFER 4096
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

		UPDATE_MESH,

		CREATE_CONST_BUFFER,
		UPDATE_CONST_BUFFER,
		
		RENDER,
		FLUSH_RENDER,
		SWAP,

		CREATE_VERTEX_SHADER,
		FREE_VERTEX_SHADER,
		CREATE_PIXEL_SHADER,
		FREE_PIXEL_SHADER,
		CREATE_PIPELINE,
		FREE_PIPELINE,

		CREATE_TEXTURE,
		CREATE_RENDER_TEXTURE,
		CREATE_SAMPLER,

		CREATE_FENCE,
		INSERT_FENCE,

		FRAME_START,
		FRAME_END,
	};

	CommandType type;

	DEBUG_ONLY(u64 frameCount);
	DEBUG_ONLY(int queueIndex);
	
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

	struct CreateConstBufferCmd
	{
		ConstBufferHandle	constBuffer;
		size_t				bufferSize;
		void*				initData;
	};

	struct UpdateConstBufferCmd
	{
		ConstBufferHandle constBuffer;
		size_t			  startOffset;
		size_t			  updateSize;
		void*			  updateData;
	};
	
	struct CreateVertexShaderCmd
	{
		VertexShaderHandle	vertexShader;
		DataBlob			data;
	};

	struct FreeVertexShaderCmd
	{
		VertexShaderHandle vertexShader;
	};
	
	struct CreatePixelShaderCmd
	{
		PixelShaderHandle	pixelShader;
		DataBlob			data;
	};

	struct FreePixelShaderCmd
	{
		PixelShaderHandle pixelShader;
	};

	struct CreatePipelineCmd
	{
		PipelineHandle	pipeline;
		PipelineData	data;
	};

	struct FreePipelineCmd
	{
		PipelineHandle pipeline;
	};

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

	struct FreeFenceCmd
	{
		FenceHandle			fence;
	};
	
	struct InsertFenceCmd
	{
		FenceHandle			fence;
	};

	struct RenderCmd
	{
		int renderListIndex;
		int magicNum;
	};

	struct SwapCmd
	{
		bool vsync;
	};

	union
	{
		ResizeBufferCmd			resizeBuffCmd;
		CreateMeshCmd			createMeshCmd;
		CreateConstBufferCmd	createConstBufferCmd;
		UpdateConstBufferCmd	updateConstBufferCmd;
		UpdateMeshCmd			updateMeshCmd;
		RenderCmd				renderCmd;
		SwapCmd					swapCmd;

		CreateVertexShaderCmd	createVSCmd;
		FreeVertexShaderCmd		freeVSCmd;
		CreatePixelShaderCmd	createPSCmd;
		FreePixelShaderCmd		freePSCmd;
		CreatePipelineCmd		createPipelineCmd;
		FreePipelineCmd			freePipelineCmd;
		CreateTextureCmd		createTextureCmd;
		CreateRenderTextureCmd	createRenderTextureCmd;
		CreateSamplerCmd		createSamplerCmd;
		
		CreateFenceCmd			createFenceCmd;
		InsertFenceCmd			insertFenceCmd;
	};
};

struct RenderCmd
{
	ConstBufferHandle	cameraConstBuffer;
	RenderTextureHandle	renderTexture;
	MeshHandle			mesh;
	ConstBufferHandle	transformConstBuffer;
	PipelineHandle		pipeline;
	RenderResource		resources;
	u32					startIndex = 0;
	u32					numIndex = 0;
};

#define MAX_RENDER_CMD 256
struct RenderCmdList
{	
	RenderCmdList() : cmdCount(0), renderInProgress(false), scratchBuffer(1024*1024, GetCurrentAllocator()) {}
	RenderCmd			cmd[MAX_RENDER_CMD];
	int					cmdCount;
	std::atomic<bool>	renderInProgress;
	StackAllocator		scratchBuffer;
};
	
#define MAX_RENDER_LIST 2
struct RenderQueue
{	
	RenderQueue()
	{
		renderList = new RenderCmdList[MAX_RENDER_LIST];
	}
	
	Renderer* renderer;
	SpscFifo<RenderThreadCmd, 256> cmdList;
	RenderCmdList*			renderList;
	DEBUG_ONLY(int threadIndex);
};

struct MeshRenderData
{
	u32			numVertices = 0;
	u32			numIndices = 0;
	u32			channelMask = 0;
};

struct ConstBufferData
{
	size_t		bufferSize;
};
	
enum RenderThreadState
{
	RENDER_THREAD_INIT = 0,
	RENDER_THREAD_RUNNING = 1,
	RENDER_THREAD_EXITING = 2,
	RENDER_THREAD_STOPPED = 3,
};

YU_GLOBAL int gRenderThreadRunningState;

bool RenderThreadRunning()
{
	return gRenderThreadRunningState != RENDER_THREAD_STOPPED;
}
	
struct Renderer
{

	enum RenderStage
	{
		RENDERER_WAIT,
		RENDERER_RENDER_FRAME,
		RENDERER_END_FRAME,
	};
	
	Renderer() : renderStage(RENDERER_WAIT)
	{
		CPUInfo cpuInfo = SystemInfo::GetCPUInfo();
		int numWorkerThread =  yu::max((i32)cpuInfo.numLogicalProcessors - (i32)2, (i32)0) + 1;
		numQueue = numWorkerThread;
		renderQueue = new RenderQueue[numQueue];

		for(int i = 0; i < numQueue; i++)
		{
			renderQueue[i].renderer = this;
			DEBUG_ONLY(renderQueue[i].threadIndex = i);
		}
	}

	void RecycleResourceIdList()
	{
		meshIdList.Free();
		constBufferIdList.Free();
		vertexShaderIdList.Free();
		pixelShaderIdList.Free();
		pipelineIdList.Free();
		textureIdList.Free();
		renderTextureIdList.Free();
		samplerIdList.Free();
		fenceIdList.Free();
	}
	
	IndexFreeList<MAX_MESH>								meshIdList;
	MeshRenderData										meshList[MAX_MESH];

	IndexFreeList<MAX_CONST_BUFFER>						constBufferIdList;
	ConstBufferData										constBufferList[MAX_CONST_BUFFER];
	
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
	int				   									numQueue;

	RenderTextureHandle									frameBuffer;

	RendererDesc										rendererDesc;


	bool												vrRendering = false;
	RenderTextureHandle									eyeRenderTexture[2];

	std::atomic<int>									renderStage;

	u64													frameCount;
};

void RendererFrameCleanup(Renderer* renderer)
{
	renderer->RecycleResourceIdList();
}

void RenderFrameStart(Renderer* renderer)
{
	assert(renderer->renderStage == Renderer::RENDERER_WAIT);
	renderer->renderStage.store(Renderer::RENDERER_RENDER_FRAME, std::memory_order_release);
	renderer->frameCount = FrameCount();
#if defined YU_DEBUG
	yu::FilterLog(LOG_MESSAGE, "frame: %d started\n", renderer->frameCount);
#endif
}

void FinalFrameStart(Renderer* renderer)
{
	renderer->renderStage.store(Renderer::RENDERER_RENDER_FRAME, std::memory_order_release);	
}

void RenderFrameEnd(Renderer* renderer)
{
	assert(renderer->renderStage == Renderer::RENDERER_RENDER_FRAME);
	renderer->renderStage.store(Renderer::RENDERER_END_FRAME, std::memory_order_release);

#if defined YU_DEBUG
	renderer->frameCount = FrameCount();	
	yu::FilterLog(LOG_MESSAGE, "frame: %d ended\n", renderer->frameCount);
#endif
}

RenderTextureHandle	GetFrameBufferRenderTexture(Renderer* renderer)
{
	return renderer->frameBuffer;
}

const RendererDesc& GetRendererDesc(Renderer* renderer)
{
	return renderer->rendererDesc;
}

int GetWorkerThreadIndex();
RenderQueue* GetThreadLocalRenderQueue()
{
	Renderer* renderer = GetRenderer();
	int workerThreadIdx = GetWorkerThreadIndex();
	return &renderer->renderQueue[workerThreadIdx];;
}

RenderQueue* GetRenderQueue(int index)
{
	Renderer* renderer = GetRenderer();
	return &renderer->renderQueue[index];
}

#if defined YU_DEBUG
int GetRenderQueueIndex(RenderQueue* queue)
{
	return queue->threadIndex;
}
#endif
	
bool RenderQueueEmpty(RenderQueue* queue)
{
	return queue->cmdList.IsEmpty();
}

YU_INTERNAL void BlockEnqueueCmd(RenderQueue* queue, RenderThreadCmd cmd)
{
	DEBUG_ONLY(cmd.frameCount = FrameCount());
	DEBUG_ONLY(cmd.queueIndex = queue->threadIndex);
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
			*((Vector3*)vertex) = meshData->position[i];
				vertex += sizeof(Vector3);
		}
		if (HasTexcoord(channelMask) && HasTexcoord(meshData->channelMask))
		{
			*((Vector2*)vertex) = meshData->texcoord[i];
			vertex += sizeof(Vector2);
		}
		if (HasColor(channelMask) && HasColor(meshData->channelMask))
		{
			*((Color*)vertex) = meshData->color[i];
			vertex += sizeof(Color);
		}
	}
	
}

//extern 
void StopRenderThread(RenderQueue* queue)
{
//	gRenderThreadRunningState = 2;
	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::STOP_RENDER_THREAD;
	BlockEnqueueCmd(queue, cmd);

	FinalFrameStart(GetRenderer());
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

ConstBufferHandle CreateConstBuffer(RenderQueue* queue, size_t bufferSize, void* initData)
{
	ConstBufferHandle constBuffer;
	constBuffer.id = queue->renderer->constBufferIdList.Alloc();
	ConstBufferData* constBufferData = queue->renderer->constBufferList + constBuffer.id;
	constBufferData->bufferSize = bufferSize;
	
	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::CREATE_CONST_BUFFER;
	cmd.createConstBufferCmd.constBuffer = constBuffer;
	cmd.createConstBufferCmd.bufferSize = bufferSize;
	cmd.createConstBufferCmd.initData = initData;
	BlockEnqueueCmd(queue, cmd);

	return constBuffer;
}

void UpdateConstBuffer(RenderQueue*queue, ConstBufferHandle constBuffer, size_t startOffset, size_t updateSize, void* updateData)
{
	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::UPDATE_CONST_BUFFER;
	cmd.updateConstBufferCmd.constBuffer = constBuffer;
	cmd.updateConstBufferCmd.startOffset = startOffset;
	cmd.updateConstBufferCmd.updateSize = updateSize;
	cmd.updateConstBufferCmd.updateData = updateData;

	BlockEnqueueCmd(queue, cmd);
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

void FreeVertexShader(RenderQueue* queue, VertexShaderHandle vs)
{
	queue->renderer->vertexShaderIdList.DeferredFree(vs.id);
	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::FREE_VERTEX_SHADER;
	cmd.freeVSCmd.vertexShader = vs;
	BlockEnqueueCmd(queue, cmd);
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

void FreePixelShader(RenderQueue* queue, PixelShaderHandle ps)
{
	queue->renderer->pixelShaderIdList.DeferredFree(ps.id);	
	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::FREE_PIXEL_SHADER;
	cmd.freePSCmd.pixelShader = ps;
	BlockEnqueueCmd(queue, cmd);
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

void FreePipeline(RenderQueue* queue, PipelineHandle pipeline)
{
	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::FREE_PIPELINE;
	cmd.freePipelineCmd.pipeline = pipeline;
	BlockEnqueueCmd(queue, cmd);
}

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

void WaitAndResetFence(RenderQueue* queue, FenceHandle fence)
{
	WaitFence(queue, fence);
	ResetFence(queue, fence);
}

void ResetFence(RenderQueue* queue, FenceHandle fence)
{
	assert(queue->renderer->fenceList[fence.id].cpuExecuted == true);
	queue->renderer->fenceList[fence.id].cpuExecuted = false;
}

	void Render(RenderQueue* queue, RenderTextureHandle renderTexture, ConstBufferHandle cameraConstBuffer, MeshHandle mesh, ConstBufferHandle transformConstBuffer, PipelineHandle pipeline, const RenderResource& resources)
{
	RenderCmdList* list;
	int listIndex;

	size_t resourceSize = 0;
	{
		resourceSize = sizeof(TextureSlot) * resources.numPsTexture;
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
	cmd.cameraConstBuffer = cameraConstBuffer;
	cmd.mesh = mesh;
	cmd.transformConstBuffer = transformConstBuffer;
	cmd.pipeline = pipeline;
	cmd.startIndex = 0;
	cmd.numIndex = meshRenderData->numIndices;

	{
		cmd.resources.numPsTexture = resources.numPsTexture;
		cmd.resources.psTextures = (TextureSlot*)list->scratchBuffer.Alloc(sizeof(TextureSlot) * resources.numPsTexture);
		memcpy(cmd.resources.psTextures, resources.psTextures, sizeof(TextureSlot) * resources.numPsTexture);
	}

	list->cmdCount++;
	
	if (list->cmdCount == MAX_RENDER_CMD)
	{
		RenderThreadCmd cmd;
		cmd.type = RenderThreadCmd::RENDER;
		cmd.renderCmd.renderListIndex = listIndex;
		cmd.renderCmd.magicNum = 0xabcd;
		queue->renderList[listIndex].renderInProgress.store(true, std::memory_order_seq_cst);
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
			cmd.type = RenderThreadCmd::FLUSH_RENDER;
			cmd.renderCmd.renderListIndex = i;
			cmd.renderCmd.magicNum = 0xabcd;
			queue->renderList[i].renderInProgress.store(true, std::memory_order_seq_cst);
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


}



