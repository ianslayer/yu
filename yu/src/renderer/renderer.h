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

struct FrameBuffer
{

};

struct CameraHandle
{
	i32 id;
};

struct MeshHandle
{
	i32 id;
};

struct BaseDoubleBufferData
{
	static int	constIndex;
};

template<class T>
struct DoubleBufferData : public BaseDoubleBufferData
{
	const T& GetConst() const
	{
		return data[constIndex];
	}

	T& GetMutable() const
	{
		return data[1-constIndex];
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
	Vector3 position = Vector3(0, 0, 0);

	//right hand coordinate
	Vector3 lookat = Vector3(-1, 0, 0);//view space +z
	Vector3 right = Vector3(0, 1, 0); //view space +x
	//Vector3 down;    //view space +y, can be derived from lookat ^ right
	Vector3 Down() const;

	float   xFov = 3.14f / 2.f;

	//projection
	float    n = 3000.f;
	float	 f = 0.1f;

	//following parameter is derived from fov, aspect ratio and near plane
	float	 l;
	float	 r;
	float	 t;
	float    b;
};


struct DoubleBufferCameraData : public DoubleBufferData < CameraData >
{
	
};

struct MeshData
{
	Vector3*	posList = nullptr;
	Vector2*	texcoordList = nullptr;
	Color*		colorList = nullptr;

	u32*		indices;

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

void			UpdateMesh(RenderQueue* queue, MeshHandle handle, 
				u32 startVert, u32 startIndex, 
				MeshData* subMeshdata);

void			CopySubMesh(RenderQueue* queue, MeshHandle handle, 
				u32 startVert, u32 numVertices, u32 startIndex, u32 numIndices, u32 channel,
				MeshData* outMeshData);

void			Render(RenderQueue* queue, CameraHandle cam, MeshHandle mesh);
void			Flush(RenderQueue* queue);
void			Swap(RenderQueue* queue, bool vsync = true);

void			InitRenderThread(const Window& win, const FrameBufferDesc& desc);

}


#endif
