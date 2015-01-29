#include "../core/thread.h"
#include "../core/system.h"
#include "../core/allocator.h"
#include "../core/log.h"
#include "../core/file.h"
#include "renderer.h"
#include "gl_utility.h"
#include "shader.h"
#include "renderer_impl.h"
#include <new>

void InitGLContext(yu::Window& win, yu::FrameBufferDesc& desc);
void SwapBuffer(yu::Window& win);
namespace yu
{

struct InitGLParams
{
	Window			win;
	FrameBufferDesc desc;
	CondVar			initGLCV;
	Mutex			initGLCS;
};

struct VertexShaderGL
{
	GLuint vsId = 0;
};

struct PixelShaderGL
{
	GLuint psId = 0;
};

struct PipelineGL
{
	GLuint programId = 0;
	GLuint cameraUBOIndex = 0;
};

struct CameraDataGL
{
	GLuint uniformBufferId = 0;
};

struct MeshDataGL
{
	GLuint vboId = 0;
	GLuint iboId = 0;
};

struct RendererGL : public Renderer
{
	VertexShaderGL		glVertexShaderList[MAX_SHADER];
	PixelShaderGL		glPixelShaderList[MAX_SHADER];
	PipelineGL			glPipelineList[MAX_PIPELINE];
	CameraDataGL			glCameraList[MAX_CAMERA];
	MeshDataGL			glMeshList[MAX_MESH];
};

RendererGL* gRenderer;

VertexShaderAPIData LoadVSFromFile(const char* path)
{
	VertexShaderAPIData vsData = {};
	size_t fileSize = FileSize(path);
	vsData.shaderSource = (GLchar*)gDefaultAllocator->Alloc(fileSize);
	ReadFile(path, vsData.shaderSource, fileSize);
	vsData.shaderSize = (GLint) fileSize;
	return vsData;
}

PixelShaderAPIData LoadPSFromFile(const char* path)
{
	PixelShaderAPIData psData = {};
	size_t fileSize = FileSize(path);
	psData.shaderSource = (GLchar*)gDefaultAllocator->Alloc(fileSize);
	ReadFile(path, psData.shaderSource, fileSize);
	psData.shaderSize = (GLint) fileSize;
	return psData;
}

struct GLCompileResult
{
	GLuint shader;
	bool		compileSuccess;
};

YU_INTERNAL GLCompileResult CompileShader(const char* sourcePath, GLchar* shaderSource, GLint shaderSize, GLuint shaderType)
{
	GLCompileResult result = {};
	if(shaderSource == nullptr)
	{
#if defined (YU_DEBUG) || defined (YU_TOOL)
		Log("shader source is empty: %s\n", sourcePath);
#endif
		return result;
	}
	
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &shaderSource, &shaderSize);
	glCompileShader(shader);

	GLint compiled;

	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (compiled != GL_FALSE)
	{
#if defined (YU_DEBUG) || defined (YU_TOOL)
		Log("compile shader: %s success\n", sourcePath);
#endif
		result.shader = shader;
		result.compileSuccess = true;
	}
	else
	{
		char errorLog[4096];
		GLint logWrittenLength = 0;
     glGetShaderInfoLog(shader, 4096, &logWrittenLength, errorLog);
#if defined (YU_DEBUG) || defined (YU_TOOL)
		Log("compile shader: %s failed\n", sourcePath);
#endif
		Log("shader compile error: \n %s \n", errorLog);
		glDeleteShader(shader);
	}
	
	return result;
}

YU_INTERNAL void ExecUpdateCameraCmd(RendererGL* renderer, CameraHandle camera, const CameraData& updateData)
{
	renderer->cameraList.Get(camera.id)->UpdateData(updateData);
	CameraData* data = &renderer->cameraList.Get(camera.id)->GetMutable();
	CameraConstant camConstant;
	camConstant.viewMatrix = data->ViewMatrix();
	camConstant.projectionMatrix = data->PerspectiveMatrixGl();
	
	CameraDataGL& glCam = renderer->glCameraList[camera.id];
	
	if(!glCam.uniformBufferId)
	{
		glGenBuffers(1, &glCam.uniformBufferId);
		glBindBuffer(GL_UNIFORM_BUFFER, glCam.uniformBufferId);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraConstant), &camConstant, GL_DYNAMIC_DRAW);
	}
	else
	{
		glBindBuffer(GL_UNIFORM_BUFFER, glCam.uniformBufferId);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(CameraConstant), &camConstant);
	}
}

YU_INTERNAL void ExecCreateMeshCmd(RendererGL* renderer, MeshHandle mesh, u32 numVertices, u32 numIndices,
u32 channelMask, MeshData* meshData)
{
	MeshDataGL& glMesh = renderer->glMeshList[mesh.id];
	assert(glMesh.vboId == 0);
	assert(glMesh.iboId == 0);
	
	glGenBuffers(1, &glMesh.vboId);
	glBindBuffer(GL_ARRAY_BUFFER, glMesh.vboId);
	glGenBuffers(1, &glMesh.iboId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glMesh.iboId);
	
	u32 vertexSize = VertexSize(channelMask);
	u32 vertexBufferSize = vertexSize * numVertices;
	u32 indexBufferSize = sizeof(u32) * numIndices;
	void* vertexBuffer = nullptr;
	void* indexBuffer = nullptr;
	if(meshData)
	{
		vertexBuffer = gDefaultAllocator->Alloc(vertexBufferSize);
		InterleaveVertexBuffer(vertexBuffer, meshData, channelMask, 0, numVertices);
		gDefaultAllocator->Free(vertexBuffer);
		indexBuffer = meshData->indices;
	}
	
	glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, vertexBuffer, GL_STREAM_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSize, indexBuffer, GL_STREAM_DRAW);
}

YU_INTERNAL void ExecUpdateMeshCmd(RendererGL* renderer, MeshHandle mesh, u32 startVert, u32 startIndex, MeshData* subMeshData)
{
	MeshDataGL& glMesh = renderer->glMeshList[mesh.id];
	glBindBuffer(GL_ARRAY_BUFFER, glMesh.vboId);
	void* mappedVertexBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	if(mappedVertexBuffer)
	{
		InterleaveVertexBuffer(mappedVertexBuffer, subMeshData, subMeshData->channelMask, startVert, subMeshData->numVertices);
	}
	else
	{
		Log("error: ExecUpdateMeshCmd gl map vertex buffer failed\n");
	}
	glUnmapBuffer(GL_ARRAY_BUFFER);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glMesh.iboId);
	void* mappedIndexBuffer = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
	if(mappedIndexBuffer)
	{
		memcpy((u32*)mappedIndexBuffer + startIndex, subMeshData->indices, sizeof(u32) * subMeshData->numIndices);
	}
	else
	{
		Log("error: ExecUpdateMeshCmd gl map index buffer failed\n");
	}
	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
}

YU_INTERNAL void ExecCreateVertexShader(RendererGL* renderer, VertexShaderHandle vertexShader, VertexShaderAPIData& shaderData)
{
	char* sourcePath = nullptr;
#if defined (YU_DEBUG) || defined (YU_TOOL)
	sourcePath = shaderData.sourcePath.str;
#endif
	GLCompileResult compileResult = CompileShader(sourcePath, shaderData.shaderSource, shaderData.shaderSize, GL_VERTEX_SHADER);
	if(compileResult.compileSuccess)
	{
		VertexShaderGL& glShader = renderer->glVertexShaderList[vertexShader.id];
		if(glShader.vsId)
		{
			glDeleteShader(glShader.vsId);
		}
		glShader.vsId = compileResult.shader;
	}
}

YU_INTERNAL void ExecCreatePixelShader(RendererGL* renderer, PixelShaderHandle pixelShader, PixelShaderAPIData& shaderData)
{
	char* sourcePath = nullptr;
#if defined (YU_DEBUG) || defined (YU_TOOL)
	sourcePath = shaderData.sourcePath.str;
#endif
	GLCompileResult compileResult = CompileShader(sourcePath, shaderData.shaderSource, shaderData.shaderSize, GL_FRAGMENT_SHADER);
	if(compileResult.compileSuccess)
	{
		PixelShaderGL& glShader = renderer->glPixelShaderList[pixelShader.id];
		if(glShader.psId)
		{
			glDeleteShader(glShader.psId);
		}
		glShader.psId = compileResult.shader;
	}
}

YU_INTERNAL void ExecCreatePipeline(RendererGL* renderer, PipelineHandle pipeline, PipelineData pipelineData)
{
	GLuint vsId = renderer->glVertexShaderList[pipelineData.vs.id].vsId;
	GLuint psId = renderer->glPixelShaderList[pipelineData.ps.id].psId;
	
	GLuint program = glCreateProgram();
	glAttachShader(program, vsId);
	glAttachShader(program, psId);
	glLinkProgram(program);
	
	GLint linkState;
	glGetProgramiv(program, GL_LINK_STATUS, &linkState);
	if(linkState)
	{
		PipelineGL& glPipeline = renderer->glPipelineList[pipeline.id];
		if(glPipeline.programId)
		{
			glDeleteProgram(glPipeline.programId);
		}
		glPipeline.programId = program;
		glPipeline.cameraUBOIndex = glGetUniformBlockIndex(program, "CameraConstant");
		if(glPipeline.cameraUBOIndex == GL_INVALID_INDEX)
		{
			Log("error: ExecCreatePipeline gl failed to get camera uniform block index\n");
		}
	}
	else
	{
		GLchar errorLog[4096];
		GLsizei errorLogWrittenLength;
		glGetProgramInfoLog(program, 4096, &errorLogWrittenLength, errorLog);
		Log("error: create pipeline failed: gl link program failed:\n %s\n", errorLog);
		glDeleteProgram(program);
	}

}

YU_INTERNAL void ExecRenderCmd(RendererGL* renderer, RenderQueue* queue, int renderListIdx)
{
	RenderList* list = &(queue->renderList[renderListIdx]);
	assert(list->renderInProgress.load(std::memory_order_acquire));

	for (int i = 0; i < list->cmdCount; i++)
	{
		RenderCmd& cmd = list->cmd[i];
		MeshHandle& mesh = cmd.mesh;
		CameraHandle& camera = cmd.cam;
		PipelineHandle& pipeline = cmd.pipeline;
		RenderResource& resources = cmd.resources;
		
		PipelineGL& glPipeline = renderer->glPipelineList[pipeline.id];
		CameraDataGL& glCamera = renderer->glCameraList[camera.id];
		MeshRenderData& meshRenderData = *(renderer->meshList.Get(mesh.id));
		MeshDataGL& glMeshData = renderer->glMeshList[mesh.id];
		
		glBindBufferBase(GL_UNIFORM_BUFFER, glPipeline.cameraUBOIndex, glCamera.uniformBufferId);
		glUseProgram(glPipeline.programId);
		
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		
		GLuint currentVerAttribLoc = 0;
		
		GLsizei stride = (GLsizei)VertexSize(meshRenderData.channelMask);
		u8* offset = 0;
		if(HasPos(meshRenderData.channelMask))
		{
			glEnableVertexAttribArray(currentVerAttribLoc);
			glVertexAttribPointer(currentVerAttribLoc, 3, GL_FLOAT, GL_FALSE, stride, offset);
			currentVerAttribLoc++;
			offset += sizeof(Vector3);
		}
		if(HasTexcoord(meshRenderData.channelMask))
		{
			glEnableVertexAttribArray(currentVerAttribLoc);
			glVertexAttribPointer(currentVerAttribLoc, 2, GL_FLOAT, GL_FALSE, stride, offset);
			currentVerAttribLoc++;
			offset += sizeof(Vector2);
		}
		if(HasColor(meshRenderData.channelMask))
		{
			glEnableVertexAttribArray(currentVerAttribLoc);
			glVertexAttribPointer(currentVerAttribLoc, 4, GL_UNSIGNED_BYTE, GL_FALSE, stride, offset);
			currentVerAttribLoc++;
			offset += sizeof(Color);
		}
		
		glBindBuffer(GL_ARRAY_BUFFER, glMeshData.vboId);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glMeshData.iboId);
		
		glDrawElements(GL_TRIANGLES, (GLsizei)meshRenderData.numIndices, GL_UNSIGNED_INT, 0);
	}
	
	list->cmdCount = 0;
	list->renderInProgress.store(false, std::memory_order_release);
}

YU_INTERNAL bool ExecThreadCmd(RendererGL* renderer)
{
	bool frameEnd = false;
	for (int i = 0; i < renderer->numQueue; i++)
	{
		RenderQueue* queue = renderer->renderQueue[i];
		RenderThreadCmd cmd;
		while (queue->cmdList.Dequeue(cmd))
		{
			switch(cmd.type)
			{
				case (RenderThreadCmd::UPDATE_CAMERA):
				{
					ExecUpdateCameraCmd(renderer, cmd.updateCameraCmd.camera, cmd.updateCameraCmd.camData);
				}break;
				case (RenderThreadCmd::CREATE_MESH):
				{
					ExecCreateMeshCmd(renderer, cmd.createMeshCmd.mesh, cmd.createMeshCmd.numVertices,
					cmd.createMeshCmd.numIndices, cmd.createMeshCmd.vertChannelMask, cmd.createMeshCmd.meshData);
				}break;
				case (RenderThreadCmd::UPDATE_MESH):
				{
					ExecUpdateMeshCmd(renderer, cmd.updateMeshCmd.mesh, cmd.updateMeshCmd.startVertex, cmd.updateMeshCmd.startIndex, cmd.updateMeshCmd.meshData);
				}break;
				case(RenderThreadCmd::CREATE_VERTEX_SHADER) :
				{
					ExecCreateVertexShader(renderer, cmd.createVSCmd.vertexShader, cmd.createVSCmd.data);
				}break;
				case(RenderThreadCmd::CREATE_PIXEL_SHADER):
				{
					ExecCreatePixelShader(renderer, cmd.createPSCmd.pixelShader, cmd.createPSCmd.data);
				}break;
				case(RenderThreadCmd::CREATE_PIPELINE):
				{
					ExecCreatePipeline(renderer, cmd.createPipelineCmd.pipeline, cmd.createPipelineCmd.data);
				}break;
				case(RenderThreadCmd::RENDER) :
				{
					ExecRenderCmd(renderer, queue, cmd.renderCmd.renderListIndex);
				}break;			
				case (RenderThreadCmd::SWAP) :
				{
					frameEnd = true;
				}break;
			}
		}
	}
	return frameEnd;
}

bool YuRunning();
ThreadReturn ThreadCall RenderThread(ThreadContext context)
{
	InitGLParams* param = (InitGLParams*) context;
	FrameBufferDesc frameBufferDesc;
	param->initGLCS.Lock();
	Window win = param->win;
	frameBufferDesc = param->desc;
	InitGLContext(param->win, param->desc);
	
	NotifyCondVar(param->initGLCV);
	param->initGLCS.Unlock();

	FrameLock* lock = AddFrameLock();
	while(YuRunning())
	{
		WaitForKick(lock);
		glClearColor(1, 0, 0, 1);
		glViewport(0, 0, frameBufferDesc.width, frameBufferDesc.height);
		glClear(GL_COLOR_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		
		while(!ExecThreadCmd(gRenderer) && YuRunning())
			;
		SwapBuffer(win);
		FrameComplete(lock);
	}
	
	return 0;
}

Thread	renderThread;
void InitRenderThread(const Window& win, const FrameBufferDesc& desc)
{
	InitGLParams param;
	param.initGLCS.Lock();
	param.win = win;
	param.desc = desc;
	renderThread = CreateThread(RenderThread, &param);
	CPUInfo cpuInfo = SystemInfo::GetCPUInfo();
	SetThreadAffinity(renderThread.handle, 1 << (cpuInfo.numLogicalProcessors - 1));
	WaitForCondVar(param.initGLCV, param.initGLCS);
	param.initGLCS.Unlock();
}

Renderer* CreateRenderer()
{
	gRenderer = New<RendererGL>(gSysArena);
	return gRenderer;
}

void FreeRenderer(Renderer* renderer)
{
	RendererGL* rendererGL = (RendererGL*) renderer;
	Delete(gSysArena, rendererGL);
}


}
