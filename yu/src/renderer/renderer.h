#ifndef YU_RENDERER_H
#define YU_RENDERER_H
#include "../math/vector.h"
#include "../math/matrix.h"
#include "color.h"
namespace yu
{
struct DisplayMode;
struct Window;
struct ConstBufferHandle { i32 id; };
struct MeshHandle{ i32 id; };
struct VertexShaderHandle{ i32 id; };
struct PixelShaderHandle{ i32 id; };
struct TextureHandle{ i32 id; };
struct RenderTextureHandle { i32 id; };
struct SamplerHandle{ i32 id; };
struct PipelineHandle{ i32 id; };
struct FenceHandle{ i32 id; };

enum TextureFormat
{
	TEX_FORMAT_UNKNOWN,
	TEX_FORMAT_R8G8B8A8_UNORM,
	TEX_FORMAT_R8G8B8A8_UNORM_SRGB,

	TEX_FORMAT_R16G16_FLOAT,
	
	NUM_TEX_FORMATS
};

struct RendererDesc
{
	TextureFormat	frameBufferFormat;
	double			refreshRate;
	int				width;
	int				height;
	int				sampleCount;
	bool			fullScreen;

	bool			initOvrRendering;
	bool			supportOvrRendering;
};

struct TextureDesc
{
	TextureFormat	format;
	int				width;
	int				height;
	int				mipLevels;
	bool			renderTexture;
};

struct TextureMipData
{
	void* texels;
	size_t texDataSize;
};

struct RenderTextureDesc
{
	TextureHandle	refTexture;
	int				mipLevel;
};

struct SamplerStateDesc
{
	enum Filter
	{
		FILTER_POINT,
		FILTER_LINEAR,
		FILTER_TRILINEAR,
	};

	enum AddressMode
	{
		ADDRESS_CLAMP,
		ADDRESS_WRAP,
	};

	Filter		filter;
	AddressMode addressU;
	AddressMode addressV;
};

struct CameraConstant
{
	Matrix4x4 viewMatrix;
	Matrix4x4 projectionMatrix;
};

struct VRCameraConstant
{
	CameraConstant eye[2];
};

struct TransformConstant
{
	Matrix4x4 transform;
};

struct MeshData
{
	Vector3*	position = nullptr;
	Vector2*	texcoord = nullptr;
	Color*		color = nullptr;

	u32*		indices = nullptr;

	u32			numVertices = 0;
	u32			numIndices = 0;

	enum Channel
	{
		POSITION = 1,
		TEXCOORD = 1 << 1,
		COLOR =		1 << 2,
	};

	u32			channelMask = 0;
};

struct PipelineData //TODO: add vertex format, render state etc
{
	VertexShaderHandle vs;
	PixelShaderHandle ps;
};

struct TextureSlot
{
	TextureHandle textures;
	SamplerHandle sampler;
};

struct RenderResource
{
	TextureSlot*	psTextures;
	u32				numPsTexture;
};

struct Renderer;
struct RenderQueue;

/*
Renderer*		CreateRenderer();
void			FreeRenderer(Renderer* renderer);
*/

Renderer*			GetRenderer();
void				RendererFrameCleanup(Renderer* renderer);
void				RenderFrameStart(Renderer* renderer);
//void				FinalFrameStart(Renderer* renderer);
void				RenderFrameEnd(Renderer* renderer);
RenderTextureHandle	GetFrameBufferRenderTexture(Renderer* renderer);

const RendererDesc& GetRendererDesc(Renderer* renderer);
RenderQueue*	GetThreadLocalRenderQueue();

#if defined YU_DEBUG
int				GetRenderQueueIndex(RenderQueue* queue);			
RenderQueue*	GetRenderQueue(int index);
bool			RenderQueueEmpty(RenderQueue* queue);
//RenderQueue*	CreateRenderQueue(Renderer* renderer);
#endif

void			StartVRRendering(RenderQueue* queue);
void			EndVRRendering(RenderQueue* queue);
void			SetVRRenderTextures(RenderTextureHandle eyeRenderTexture[2]);
Vector2i		GetVRTextureSize(int eye);
float			GetHmdEyeHeight();

ConstBufferHandle CreateConstBuffer(RenderQueue* queue, size_t bufferSize, void* initData);
void			UpdateConstBuffer(RenderQueue* queue, ConstBufferHandle buffer, size_t startOffset, size_t updateSize, void* updateData);
	
MeshHandle		CreateMesh(RenderQueue* queue, u32 numVertices, u32 numIndices, u32 vertChannelMask);
MeshHandle		CreateMesh(RenderQueue* queue, MeshData* meshData, bool copyData = false);

//MeshHandle	CreateSubMesh(RenderQueue* queue, MeshHandle mainMesh, MeshData* subMeshdata); //?

void			FreeMesh(RenderQueue* queue, MeshHandle handle);

void			UpdateMesh(RenderQueue* queue, MeshHandle handle,
				u32 startVert, u32 startIndex,
				MeshData* subMeshdata);

struct DataBlob;
VertexShaderHandle	CreateVertexShader(RenderQueue* queue, const DataBlob& shaderData);
void				FreeVertexShader(RenderQueue* queue, VertexShaderHandle vs);
PixelShaderHandle	CreatePixelShader(RenderQueue* queue, const DataBlob& shaderData);
void				FreePixelShader(RenderQueue* queue, const PixelShaderHandle ps);
void CreateShaderCache(const char* shaderPath, const char* entryPoint, const char* profile);
PipelineHandle		CreatePipeline(RenderQueue* queue, const PipelineData& pipelineData);
void				FreePipeline(RenderQueue* queue, PipelineHandle pipeline);

TextureHandle		CreateTexture(RenderQueue* queue, const TextureDesc& desc, TextureMipData* initData = nullptr);
void				FreeTexture(TextureHandle texture);
size_t				TextureSize(TextureFormat format, int width, int height, int depth, int mipLevels);
size_t				TextureLevelSize(TextureFormat format, int width, int height, int depth, int mipSlice);
RenderTextureHandle	CreateRenderTexture(RenderQueue* queue, const RenderTextureDesc& desc);
void				FreeRenderTexture(RenderTextureHandle renderTexture);
SamplerHandle		CreateSampler(RenderQueue* queue, const SamplerStateDesc& desc);

FenceHandle			CreateFence(RenderQueue* queue); //TODO: implement true gpu fence, bool createGpuFence = false
void				InsertFence(RenderQueue* queue, FenceHandle fence);
bool				IsCPUComplete(RenderQueue* queue, FenceHandle fence);
//TODO: Implement IsGPUComplete
void				WaitFence(RenderQueue* queue, FenceHandle fence);
void				WaitAndResetFence(RenderQueue* queue, FenceHandle fence);
void				ResetFence(RenderQueue* queue, FenceHandle fence);
void				FreeFence(RenderQueue* queue, FenceHandle fence);
void				WaitRenderThreadComplete(RenderQueue* queue);

struct RenderCmdList*	CreateRenderCmdList(class Allocator* allocator, int numCmd);

void				BeginRenderPass(RenderQueue* queue, RenderTextureHandle renderTexture);
void				EndRenderPass(RenderQueue* queue);

void				Render(RenderQueue* queue, RenderTextureHandle renderTexture, ConstBufferHandle cameraConstBuffer, MeshHandle mesh, ConstBufferHandle transformConstBuffer, PipelineHandle pipeline, const RenderResource& resource);

	/*
void				Render(RenderQueue* queue, RenderTextureHandle renderTexture, CameraData* cam, MeshHandle mesh, PipelineHandle pipeline, const RenderResource& resource);
	*/

void				Flush(RenderQueue* queue);
//void				Swap(RenderQueue* queue, bool vsync = true);

void				InitRenderThread(const Window& win, const RendererDesc& desc, Allocator* allocator);
void				StopRenderThread(RenderQueue* queue);
bool 				RenderThreadRunning();
}

#define YU_FORCE_GL
#if defined YU_OS_MAC || defined YU_FORCE_GL
#	define YU_GL
#	define YU_GRAPHICS_API YU_GL
#elif defined YU_OS_WIN32
#	define YU_DX11
#	define YU_GRAPHICS_API YU_DX11
#endif


#endif
