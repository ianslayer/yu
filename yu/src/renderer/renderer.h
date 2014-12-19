#ifndef YU_RENDERER_H
#define YU_RENDERER_H
#include "../math/vector.h"
#include "../math/matrix.h"
#include "color.h"
namespace yu
{
struct DisplayMode;
struct Window;

enum TexFormat
{
	TEX_FORMAT_UNKNOWN,
	TEX_FORMAT_R8G8B8A8_UNORM,
	TEX_FORMAT_R8G8B8A8_UNORM_SRGB,

	NUM_TEX_FORMATS
};

struct FrameBufferDesc
{
	TexFormat	format;
	double		refreshRate;
	int			width;
	int			height;
	int			sampleCount;
	bool		fullScreen;
};

struct CameraHandle{ i32 id; };
struct MeshHandle{ i32 id; };
struct VertexShaderHandle{ i32 id; };
struct PixelShaderHandle{ i32 id; };
struct PipelineHandle{ i32 id; };
struct FenceHandle{ i32 id; };

struct BaseDoubleBufferData
{
	BaseDoubleBufferData* nextDirty;
	static BaseDoubleBufferData* dirtyLink;

	u32 updateCount = 0;
	bool dirty;

	static void SwapDirty()
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
		return data[updateCount&1];
	}

	T& GetMutable()
	{
		return data[1-updateCount&1];
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

struct CameraData
{
	Matrix4x4 ViewMatrix() const;
	Matrix4x4 InvViewMatrix() const;
	Matrix4x4 PerspectiveMatrix() const;
	Matrix4x4 InvPerspectiveMatrix() const;

	Matrix4x4 ScreenToRasterMatrix() const;
	Matrix4x4 RasterToScreenMatrix() const;
	Matrix4x4 RasterToCameraMatrix() const;

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

struct DoubleBufferCameraData : public DoubleBufferData < CameraData >
{

};

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

struct PipelineData
{
	VertexShaderHandle vs;
	PixelShaderHandle ps;
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
void			FreeMesh(RenderQueue* queue, MeshHandle handle);

struct VertexShaderAPIData;
struct PixelShaderAPIData;

VertexShaderHandle	CreateVertexShader(RenderQueue* queue, const VertexShaderAPIData& data);
PixelShaderHandle	CreatePixelShader(RenderQueue* queue, const PixelShaderAPIData& data);

PipelineHandle		CreatePipeline(RenderQueue* queue, const PipelineData& data);

void			UpdateMesh(RenderQueue* queue, MeshHandle handle, 
				u32 startVert, u32 startIndex, 
				MeshData* subMeshdata);

void			CopySubMesh(RenderQueue* queue, MeshHandle handle, 
				u32 startVert, u32 numVertices, u32 startIndex, u32 numIndices, u32 channel,
				MeshData* outMeshData);

void			Render(RenderQueue* queue, CameraHandle cam, MeshHandle mesh, PipelineHandle pipeline);

void			Flush(RenderQueue* queue);
void			Swap(RenderQueue* queue, bool vsync = true);

FenceHandle		CreateFence();
void			InsertFence(FenceHandle fence);
bool			Complete(FenceHandle fence);
void			Reset(FenceHandle fence);

void			InitRenderThread(const Window& win, const FrameBufferDesc& desc);

}


#endif
