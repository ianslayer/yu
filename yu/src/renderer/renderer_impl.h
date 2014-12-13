#include "renderer.h"
#include "../container/dequeue.h"
#include "../core/free_list.h"

namespace yu
{

Matrix4x4 CameraData::ViewMatrix() const
{
	Vector3 down = Down();
	return Matrix4x4(right.x, right.y, right.z,0,
					 down.x, down.y, down.z, 0,
					 lookat.x, lookat.y, lookat.z, 0,
					 0, 0, 0, 1) * Translate(-position);
}
Matrix4x4 CameraData::InvViewMatrix() const
{
	Matrix4x4 viewMatrix = ViewMatrix(); 
	return InverseAffine(viewMatrix);
}

Vector3 CameraData::Down() const
{
	return cross(lookat, right);
}

void CameraData::SetXFov(float angleRad, float filmWidth, float filmHeight)
{
	xFov = angleRad;
	DeriveProjectionParamter(filmWidth, filmHeight);
}

void CameraData::DeriveProjectionParamter(float filmWidth, float filmHeight)
{
	r = f* tanf(xFov/2.f);
	t = r * filmHeight / filmHeight;

	l = -r;
	b = -t;
}

Matrix4x4 CameraData::PerspectiveMatrix() const
{

	float width = r - l;
	float height = t - b;

	//use complementary z
	float zNear = n;
	float zFar =f;

	float depth = zFar - zNear;

	return Scale(Vector3(1, -1, 1)) * Matrix4x4(2.f * zFar / width, 0.f, -(l + r) / width, 0.f,
					0.f, 2.f * zFar / height, -(t + b) / height,0.f,
					0.f, 0.f, zFar / depth, -(zFar * zNear) / depth,
					0.f, 0.f, 1.f, 0.f);
}

#define MAX_CAMERA 256
#define MAX_MESH 4096
#define MAX_RENDER_QUEUE 32

struct RenderThreadCmd
{
	enum CommandType
	{
		RESIZE_BUFFER,
		UPDATE_MESH,
		UPDATE_CAMERA,
		RENDER,
		SWAP,
	};

	CommandType type;

	struct ResizeBufferCmd
	{
		unsigned int	width;
		unsigned int	height;
		TexFormat		fmt;
	};

	//TODO: merge the below two update into render cmd
	struct UpdateMeshCmd
	{
		MeshHandle handle;
	};

	struct UpdateCameraCmd
	{
		CameraHandle handle;
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
		ResizeBufferCmd resizeBuffCmd;
		UpdateCameraCmd	updateCameraCmd;
		UpdateMeshCmd	updateMeshCmd;
		RenderCmd		renderCmd;
		SwapCmd			swapCmd;
	};
};

struct RenderCmd
{
	CameraHandle	cam;
	MeshHandle		mesh;
	u32				startIndex = 0;
	u32				numIndex = 0;
};

#define MAX_RENDER_CMD 256
struct RenderList
{
	RenderCmd			cmd[MAX_RENDER_CMD];
	int					cmdCount;
	std::atomic<bool>	renderInProgress;
};
void Clear(RenderList& list)
{
	list.cmdCount = 0;
	list.renderInProgress.store(false, std::memory_order_release);
}

#define MAX_RENDER_LIST 2
struct RenderQueue
{
	Renderer* renderer;
	SpscFifo<RenderThreadCmd, 256> cmdList;

	RenderList			renderList[MAX_RENDER_LIST];
};

struct Renderer
{
	FreeList<CameraData, MAX_CAMERA>		cameraList;
	FreeList<MeshData, MAX_MESH>			meshList;
	FreeList<RenderQueue, MAX_RENDER_QUEUE>	renderQueueList; //generally one queue per worker thread
	RenderQueue								*renderQueue[MAX_RENDER_QUEUE];
	int										numQueue = 0;
};

RenderQueue* CreateRenderQueue(Renderer* renderer)
{
	int queueIdx = renderer->renderQueueList.Alloc();
	RenderQueue* queue = renderer->renderQueueList.Get(queueIdx);
	renderer->renderQueue[renderer->numQueue] = queue;
	renderer->numQueue++;

	queue->renderer = renderer;
	return queue;
}

MeshHandle	CreateMesh(RenderQueue* queue, u32 numVertices, u32 numIndices, u32 vertChannelMask)
{
	MeshHandle handle;
	handle.id = queue->renderer->meshList.Alloc();
	return handle;
}

void FreeMesh(RenderQueue* queue, MeshHandle handle)
{
	MeshData* data = queue->renderer->meshList.Get(handle.id);
	delete data->posList; data->posList = nullptr;
	delete data->texcoordList; data->texcoordList = nullptr;
	delete data->colorList; data->colorList = nullptr;
	queue->renderer->meshList.Free(handle.id);
}

bool HasPos(u32 mask)
{
	return (mask & MeshData::POSITION) != 0;
}

bool HasTexcoord(u32 mask)
{
	return (mask & MeshData::TEXCOORD) != 0;
}

bool HasColor(u32 mask)
{
	return (mask & MeshData::COLOR) != 0;
}

void UpdateMesh(RenderQueue* queue, MeshHandle handle, 
	u32 startVert, u32 startIndex, 
	MeshData* inputSubMesh)
{
	Renderer* renderer = queue->renderer;
	//TODO: should use an update queue, so it's thread safe
	MeshData* data = renderer->meshList.Get(handle.id);

	if ((startVert + inputSubMesh->numVertices) > data->numVertices || data->channelMask != inputSubMesh->channelMask)
	{
		Vector3* posList = nullptr;
		Vector2* texcoordList = nullptr;
		Color*	colorList = nullptr;

		if (HasPos(inputSubMesh->channelMask))
		{
			posList = new Vector3[startVert + inputSubMesh->numVertices];
			if (data->posList)
				memcpy(posList, data->posList, sizeof(Vector3) * data->numVertices);
		}
		if (HasTexcoord(inputSubMesh->channelMask))
		{
			texcoordList = new Vector2[startVert + inputSubMesh->numVertices];
			if (data->colorList)
				memcpy(texcoordList, data->texcoordList, sizeof(Vector2) * data->numVertices);
		}
		if (HasColor(inputSubMesh->channelMask))
		{
			colorList = new Color[startVert + inputSubMesh->numVertices];
			if (data->colorList)
				memcpy(colorList, data->colorList, sizeof(Color) * data->numVertices);
		}

		delete data->posList; data->posList = posList;
		delete data->texcoordList; data->texcoordList = texcoordList;
		delete data->colorList; data->colorList = colorList;

		data->numVertices = startVert + inputSubMesh->numVertices;
		data->channelMask = inputSubMesh->channelMask;
	}

	if (HasPos(inputSubMesh->channelMask))
	{
		memcpy(data->posList + sizeof(Vector3) * startVert, inputSubMesh->posList, sizeof(Vector3) *inputSubMesh->numVertices);
	}
	if (HasTexcoord(inputSubMesh->channelMask))
	{
		memcpy(data->texcoordList + sizeof(Vector2) * startVert, inputSubMesh->texcoordList, sizeof(Vector2)  * inputSubMesh->numVertices);
	}
	if (HasColor(inputSubMesh->channelMask))
	{
		memcpy(data->colorList + sizeof(Color) * startVert, inputSubMesh->colorList, sizeof(Color)  * inputSubMesh->numVertices);
	}

	if (inputSubMesh->indices)
	{
		if ((startIndex + inputSubMesh->numIndices) > data->numIndices)
		{
			u32* indices = new u32[(startIndex + inputSubMesh->numIndices)];
			if (data->indices)
				memcpy(indices, data->indices, sizeof(u32) * (startIndex + inputSubMesh->numIndices));
			delete data->indices;
			data->indices = indices;
		}

		data->numIndices = (startIndex + inputSubMesh->numIndices);
		memcpy(data->indices + sizeof(u32) * startIndex, inputSubMesh->indices, sizeof(u32) * inputSubMesh->numIndices);
	}

	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::UPDATE_MESH;
	cmd.updateMeshCmd.handle = handle;
	while (!queue->cmdList.Enqueue(cmd))
		;
}

void UpdateCamera(RenderQueue* queue, CameraHandle handle, const CameraData& camera)
{
	Renderer* renderer = queue->renderer;
	//TODO: should use an update queue, so it's thread safe
	CameraData* data = renderer->cameraList.Get(handle.id);

	*data = camera;

	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::UPDATE_CAMERA;
	cmd.updateCameraCmd.handle = handle;
	while (!queue->cmdList.Enqueue(cmd))
		;
}

void Render(RenderQueue* queue, CameraHandle cam, MeshHandle mesh)
{
	RenderList* list;
	int listIndex;
	for (;;)
	{
		for (int i = 0; i < MAX_RENDER_LIST; i++)
		{
			if (!queue->renderList[i].renderInProgress.load(std::memory_order_acquire))
			{
				list = &queue->renderList[i];
				listIndex = i;
				goto ListFound;
			}
		}
	}
ListFound:
	
	MeshData* meshData = queue->renderer->meshList.Get(mesh.id);

	list->cmd[list->cmdCount].cam = cam;
	list->cmd[list->cmdCount].mesh = mesh;
	list->cmd[list->cmdCount].startIndex = 0;
	list->cmd[list->cmdCount].numIndex = meshData->numIndices;
	list->cmdCount++;
	
	if (list->cmdCount == MAX_RENDER_CMD)
	{
		RenderThreadCmd cmd;
		cmd.type = RenderThreadCmd::RENDER;
		cmd.renderCmd.renderListIndex = listIndex;
		queue->renderList[listIndex].renderInProgress.store(true, std::memory_order_release);
		while (!queue->cmdList.Enqueue(cmd))
			;
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
			while (!queue->cmdList.Enqueue(cmd))
				;
		}
	}
}

void Swap(RenderQueue* queue, bool vsync)
{
	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::SWAP;
	cmd.swapCmd.vsync = vsync;
	while (!queue->cmdList.Enqueue(cmd))
		;
}

CameraHandle CreateCamera(RenderQueue* queue)
{
	CameraHandle handle;
	handle.id = queue->renderer->cameraList.Alloc();
	return handle;
}

CameraData GetCameraData(RenderQueue* queue, CameraHandle handle)
{
	return *queue->renderer->cameraList.Get(handle.id);
}

void CopyMesh(Renderer* renderer, MeshHandle handle, MeshData* outData, u32 channel);

}



