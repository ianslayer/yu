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
struct SamplerHandle{ i32 id; };
struct PipelineHandle{ i32 id; };
struct FenceHandle{ i32 id; };

enum TextureFormat
{
	TEX_FORMAT_UNKNOWN,
	TEX_FORMAT_R8G8B8A8_UNORM,
	TEX_FORMAT_R8G8B8A8_UNORM_SRGB,

	NUM_TEX_FORMATS
};

struct FrameBufferDesc
{
	TextureFormat	format;
	double			refreshRate;
	int				width;
	int				height;
	int				sampleCount;
	bool			fullScreen;
};

struct TextureDesc
{
	TextureFormat	format;
	int				width;
	int				height;
	int				mipLevels;
};

struct TextureMipData
{
	void* texels;
	size_t texDataSize;
};

struct RenderTextureDesc
{
	TextureHandle refTexture;
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

struct CameraData
{
	Matrix4x4 ViewMatrix() const;
	Matrix4x4 InvViewMatrix() const;
	Matrix4x4 PerspectiveMatrixDx() const;
	Matrix4x4 InvPerspectiveMatrixDx() const;
	Matrix4x4 PerspectiveMatrixGl() const;
	Matrix4x4 InvPerspectiveMatrixGl() const;
	
	void      SetXFov(float angleRad, float filmWidth, float filmHeight);

	void      DeriveProjectionParamter(float filmWidth, float filmHeight);

	// view transform
	Vector3 position ;

	//right hand coordinate
	Vector3 lookat ;//view space +z
	Vector3 right ; //view space +x
	//Vector3 down;    //view space +y, can be derived from lookat ^ right
	Vector3 Down() const;

	float   xFov ;

	//projection
	float    n;
	float	 f;

	//following parameter is derived from fov, aspect ratio and near plane
	float	 l;
	float	 r;
	float	 t;
	float    b;
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

struct VertexShaderData
{
#if defined YU_DEBUG || defined YU_TOOL
	StringRef sourcePath;
#endif
};

struct PixelShaderData
{
#if defined YU_DEBUG || defined YU_TOOL
	StringRef sourcePath;
#endif
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

Renderer*		CreateRenderer();
void			FreeRenderer(Renderer* renderer);
RenderQueue*	CreateRenderQueue(Renderer* renderer);

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


struct VertexShaderAPIData;
struct PixelShaderAPIData;

VertexShaderHandle	CreateVertexShader(RenderQueue* queue, const VertexShaderAPIData& data);
PixelShaderHandle	CreatePixelShader(RenderQueue* queue, const PixelShaderAPIData& data);

PipelineHandle		CreatePipeline(RenderQueue* queue, const PipelineData& data);

TextureHandle		CreateTexture(RenderQueue* queue, const TextureDesc& desc, TextureMipData* initData = nullptr);
size_t				TextureSize(TextureFormat format, int width, int height, int depth, int mipLevels);
size_t				TextureLevelSize(TextureFormat format, int width, int height, int depth, int mipSlice);
SamplerHandle		CreateSampler(RenderQueue* queue, const SamplerStateDesc& desc);

FenceHandle			CreateFence(RenderQueue* queue); //TODO: implement true gpu fence, bool createGpuFence = false
void				InsertFence(RenderQueue* queue, FenceHandle fence);
bool				IsCPUComplete(RenderQueue* queue, FenceHandle fence);
//TODO: Implement IsGPUComplete
void				WaitFence(RenderQueue* queue, FenceHandle fence);
void				Reset(RenderQueue* queue, FenceHandle fence);

void				Render(RenderQueue* queue, CameraHandle cam, MeshHandle mesh, PipelineHandle pipeline, const RenderResource& resource);

void				Flush(RenderQueue* queue);
void				Swap(RenderQueue* queue, bool vsync = true);

void				InitRenderThread(const Window& win, const FrameBufferDesc& desc);

}

#if defined YU_OS_MAC || defined YU_FORCE_GL
#	define YU_GL
#	define YU_GRAPHICS_API YU_GL
#elif defined YU_OS_WIN32
#	define YU_DX11
#	define YU_GRAPHICS_API YU_DX11
#endif


#endif
