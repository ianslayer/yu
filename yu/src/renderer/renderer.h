#ifndef YU_RENDERER_H
#define YU_RENDERER_H
#include "../math/vector.h"
#include "../math/matrix.h"
#include "color.h"
namespace yu
{
struct DisplayMode;
struct Window;
struct CameraHandle{ i32 id; };
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

Matrix4x4 ViewMatrix(Vector3 pos, Vector3 lookAt, Vector3 right);
Matrix4x4 PerspectiveMatrixDX(float halfTanX, float n, float f, float filmWidth, float filmHeight); //z range 0~1, right handed
Matrix4x4 PerspectiveMatrixDX(float upTan, float downTan, float leftTan, float rightTan, float n, float f);
Matrix4x4 PerspectiveMatrixGL(float halfTanX, float n, float f, float filmWidth, float filmHeight); //z range -1~1, right handed
Matrix4x4 PerspectiveMatrixGL(float upTan, float downTan, float leftTan, float rightTan, float n, float f);

struct CameraData
{
	// view transform
	Vector3 position;

	//right hand coordinate
	Vector3 lookAt;//view space +z
	Vector3 right; //view space +x

	float upTan;
	float downTan;
	float leftTan;
	float rightTan;

	float	filmWidth;
	float	filmHeight;

	float    n;
	float	 f;
};

CameraData DefaultCamera();

struct MeshData
{
	Vector3*	posList = nullptr;
	Vector2*	texcoordList = nullptr;
	Color*		colorList = nullptr;

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

struct RenderResource
{
	struct TextureSlot
	{
		TextureHandle textures;
		SamplerHandle sampler;
	};
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
RenderTextureHandle	GetFrameBufferRenderTexture(Renderer* renderer);

const RendererDesc& GetRendererDesc(Renderer* renderer);
RenderQueue*	GetThreadLocalRenderQueue();
RenderQueue*	CreateRenderQueue(Renderer* renderer);

void			StartVRRendering(RenderQueue* queue);
void			EndVRRendering(RenderQueue* queue);
void			SetVRRenderTextures(RenderTextureHandle eyeRenderTexture[2]);
Vector2i		GetVRTextureSize(int eye);
float			GetHmdEyeHeight();

CameraHandle	CreateCamera(RenderQueue* queue);
CameraData		GetCameraData(RenderQueue* queue, CameraHandle handle);
void			UpdateCamera(RenderQueue* queue, CameraHandle handle, const CameraData& camData);
void			FreeCamera(RenderQueue* queue, CameraHandle handle);

MeshHandle		CreateMesh(RenderQueue* queue, u32 numVertices, u32 numIndices, u32 vertChannelMask);
MeshHandle		CreateMesh(RenderQueue* queue, u32 vertChannelMask, MeshData* meshData, bool copyData = false);

//MeshHandle	CreateSubMesh(RenderQueue* queue, MeshHandle mainMesh, MeshData* subMeshdata); //?

void			FreeMesh(RenderQueue* queue, MeshHandle handle);

void			UpdateMesh(RenderQueue* queue, MeshHandle handle,
				u32 startVert, u32 startIndex,
				MeshData* subMeshdata);

struct DataBlob;
VertexShaderHandle	CreateVertexShader(RenderQueue* queue, const DataBlob& shaderData);
PixelShaderHandle	CreatePixelShader(RenderQueue* queue, const DataBlob& shaderData);

PipelineHandle		CreatePipeline(RenderQueue* queue, const PipelineData& pipelineData);

#if defined (YU_DEBUG) || defined (YU_TOOL)
void				ReloadVertexShader(RenderQueue* queue, VertexShaderHandle vs, const DataBlob& data);
void				ReloadPixelShader(RenderQueue* queue, PixelShaderHandle ps, const DataBlob& data);
void				ReloadPipeline(RenderQueue* queue, PipelineHandle& pipeline, const PipelineData& pipelineData);
#endif

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
void				Reset(RenderQueue* queue, FenceHandle fence);

void				Render(RenderQueue* queue, RenderTextureHandle renderTexture, CameraHandle cam, MeshHandle mesh, PipelineHandle pipeline, const RenderResource& resource);

void				Render(RenderQueue* queue, RenderTextureHandle eyeTexture[2], CameraHandle eye[2], MeshHandle mesh, PipelineHandle pipeline, const RenderResource& resource);

void				Flush(RenderQueue* queue);
void				Swap(RenderQueue* queue, bool vsync = true);

void				InitRenderThread(const Window& win, const RendererDesc& desc);
void				StopRenderThread(RenderQueue* queue);

}

#if defined YU_OS_MAC || defined YU_FORCE_GL
#	define YU_GL
#	define YU_GRAPHICS_API YU_GL
#elif defined YU_OS_WIN32
#	define YU_DX11
#	define YU_GRAPHICS_API YU_DX11
#endif


#endif
