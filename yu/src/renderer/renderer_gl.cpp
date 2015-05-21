#include "renderer.h"
#if defined YU_GL

#include "../core/thread.h"
#include "../core/system.h"
#include "../core/allocator.h"
#include "../core/log.h"
#include "../core/file.h"
#include "renderer.h"
#include "gl_utility.h"
#include "renderer_impl.h"
#include <new>

void InitGLContext(const yu::Window& win, const yu::RendererDesc& desc);
void SwapBuffer(yu::Window& win);
namespace yu
{

struct InitGLParams
{
	Window			win;
	RendererDesc	desc;
	CondVar			initGLCV;
	Mutex			initGLCS;
	Allocator*		allocator;
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
	GLuint vaoId = 0;
};

struct TextureGL
{
	GLuint texId = 0;
};

struct RenderTextureGL
{
	GLuint	fboId = 0;
	bool	dirty = true;
};

struct RendererGL : public Renderer
{
	RendererGL(Allocator* allocator) : Renderer(allocator)
	{
	}
		
	VertexShaderGL		glVertexShaderList[MAX_SHADER];
	PixelShaderGL		glPixelShaderList[MAX_SHADER];
	PipelineGL			glPipelineList[MAX_PIPELINE];
	CameraDataGL		glCameraList[MAX_CAMERA];
	MeshDataGL			glMeshList[MAX_MESH];
	TextureGL			glTextureList[MAX_TEXTURE];
	RenderTextureGL		glRenderTextureList[MAX_RENDER_TEXTURE];
	
	RenderTextureHandle	currentRenderTexture;
	
};

RendererGL* gRenderer;

Renderer* GetRenderer()
{
	assert(gRenderer);
	return gRenderer;
}

YU_INTERNAL void ExecUpdateCameraCmd(RendererGL* renderer, CameraHandle camera, const CameraData& updateData)
{
	renderer->cameraDataList[camera.id].UpdateData(updateData);
	CameraData* data = &renderer->cameraDataList[camera.id].GetMutable();
	CameraConstant camConstant;
	camConstant.viewMatrix =  Transpose(ViewMatrix(data->position, data->lookAt, data->right));
	camConstant.projectionMatrix = Transpose(PerspectiveMatrixGL(data->leftTan, data->n, data->f, data->filmWidth, data->filmHeight) );
	
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

YU_INTERNAL void ExecCreateMeshCmd(RendererGL* renderer, MeshHandle mesh, u32 numVertices, u32 numIndices, u32 channelMask, MeshData* meshData, Allocator* allocator)
{
	MeshDataGL& glMesh = renderer->glMeshList[mesh.id];
	assert(glMesh.vboId == 0);
	assert(glMesh.iboId == 0);
	
	glGenVertexArrays(1, &glMesh.vaoId);
	glBindVertexArray(glMesh.vaoId);
	glGenBuffers(1, &glMesh.vboId);
	glBindBuffer(GL_ARRAY_BUFFER, glMesh.vboId);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	
	GLuint currentVerAttribLoc = 0;
		
	GLsizei stride = (GLsizei)VertexSize(channelMask);
	u8* offset = 0;
	if(HasPos(channelMask))
	{
		glEnableVertexAttribArray(currentVerAttribLoc);
		glVertexAttribPointer(currentVerAttribLoc, 3, GL_FLOAT, GL_FALSE, stride, offset);
		currentVerAttribLoc++;
		offset += sizeof(Vector3);
	}
	if(HasTexcoord(channelMask))
	{
		glEnableVertexAttribArray(currentVerAttribLoc);
		glVertexAttribPointer(currentVerAttribLoc, 2, GL_FLOAT, GL_FALSE, stride, offset);
		currentVerAttribLoc++;
		offset += sizeof(Vector2);
	}
	if(HasColor(channelMask))
	{
		glEnableVertexAttribArray(currentVerAttribLoc);
		glVertexAttribPointer(currentVerAttribLoc, 4, GL_UNSIGNED_BYTE, GL_FALSE, stride, offset);
		currentVerAttribLoc++;
		offset += sizeof(Color);
	}
	
	glGenBuffers(1, &glMesh.iboId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glMesh.iboId);
	
	u32 vertexSize = VertexSize(channelMask);
	u32 vertexBufferSize = vertexSize * numVertices;
	u32 indexBufferSize = sizeof(u32) * numIndices;
	void* vertexBuffer = nullptr;
	void* indexBuffer = nullptr;
	if(meshData)
	{
		vertexBuffer = allocator->Alloc(vertexBufferSize);
		InterleaveVertexBuffer(vertexBuffer, meshData, channelMask, 0, numVertices);
		allocator->Free(vertexBuffer);
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

YU_INTERNAL GLenum GLSizedTexFormat(TextureFormat fmt)
{
	switch (fmt)
	{
		case TEX_FORMAT_R8G8B8A8_UNORM: return GL_RGBA8;
		case TEX_FORMAT_R8G8B8A8_UNORM_SRGB: return GL_SRGB8_ALPHA8;
		case TEX_FORMAT_R16G16_FLOAT: return GL_RG16F;
		case TEX_FORMAT_UNKNOWN:
		case NUM_TEX_FORMATS:
			;
		
	}
	Log("error, GLSizedTexFormat: unknown texture format\n");
	return 0;
}

struct TexFormatGL
{
	GLenum format;
	GLenum type;
};

YU_INTERNAL TexFormatGL GLTexFormat(TextureFormat fmt)
{
	TexFormatGL glFormat = {};
	switch (fmt)
	{
		case TEX_FORMAT_R8G8B8A8_UNORM:
		{
			glFormat.format = GL_RGBA;
			glFormat.type = GL_UNSIGNED_BYTE;
		}break;
		case TEX_FORMAT_R8G8B8A8_UNORM_SRGB:
		{
			glFormat.format = GL_SRGB_ALPHA; 		//TODO: is this correct?
			glFormat.type = GL_UNSIGNED_BYTE;
		}break;
		case TEX_FORMAT_R16G16_FLOAT:
		{
			glFormat.format = GL_RG16;
			glFormat.type = GL_HALF_FLOAT;
		}break;
		default:
		{
			Log("error, GLTexFormat: unknown texture format\n");
		}
	}
	return glFormat;
}

YU_INTERNAL void ExecCreateTextureCmd(RendererGL*renderer, TextureHandle texture, const TextureDesc& texDesc, const TextureMipData* initData)
{
	TextureGL& textureGL = renderer->glTextureList[texture.id];
	glGenTextures(1, &textureGL.texId);
	glBindTexture(GL_TEXTURE_2D, textureGL.texId);
	glTexStorage2D(GL_TEXTURE_2D, texDesc.mipLevels, GLSizedTexFormat(texDesc.format), (GLsizei) texDesc.width, (GLsizei) texDesc.height);
	
	if (initData)
	{
		int mipWidth = texDesc.width;
		int mipHeight = texDesc.height;
		TexFormatGL glTexFormat = GLTexFormat(texDesc.format);
		for (int mipLevel = 0; mipLevel < texDesc.mipLevels; mipLevel++)
		{
			glTexSubImage2D(GL_TEXTURE_2D,
				mipLevel, 0, 0, 
				(GLsizei)mipWidth, (GLsizei)mipHeight,
				glTexFormat.format, glTexFormat.type, 
				initData[mipLevel].texels);

			mipWidth /= 2;
			mipHeight /= 2;
		}
	}
}

YU_INTERNAL void ExecCreateRenderTexture(RendererGL* renderer, RenderTextureHandle renderTexture, const RenderTextureDesc& rtDesc)
{
	GLuint fboId;
	glGenFramebuffers(1, &fboId);
	renderer->glRenderTextureList[renderTexture.id].fboId = fboId;
}

struct GLCompileResult
{
	GLuint	shader;
	bool	compileSuccess;
};

YU_INTERNAL GLCompileResult CompileShader(const char* sourcePath, GLchar* shaderSource, GLint shaderSize, GLuint shaderType)
{
	GLCompileResult result = {};
	if(shaderSource == nullptr || shaderSize == 0)
	{
		DEBUG_ONLY(Log("shader source is empty: %s\n", sourcePath));
		return result;
	}
	
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &shaderSource, &shaderSize);
	glCompileShader(shader);

	GLint compiled;

	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (compiled != GL_FALSE)
	{
		DEBUG_ONLY(Log("compile shader: %s success\n", sourcePath));
		result.shader = shader;
		result.compileSuccess = true;
	}
	else
	{
		char errorLog[4096];
		GLint logWrittenLength = 0;
		glGetShaderInfoLog(shader, 4096, &logWrittenLength, errorLog);

		DEBUG_ONLY(Log("compile shader: %s failed\n", sourcePath));
		Log("shader compile error: \n %s \n", errorLog);
		glDeleteShader(shader);
	}
	
	return result;
}

YU_INTERNAL void ExecCreateVertexShader(RendererGL* renderer, VertexShaderHandle vertexShader, const DataBlob& shaderData)
{
	const char* sourcePath = nullptr;

	DEBUG_ONLY(sourcePath = shaderData.sourcePath.str);

	GLCompileResult compileResult = CompileShader(sourcePath, (GLchar*)shaderData.data, (GLint)shaderData.dataLen, GL_VERTEX_SHADER);
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

YU_INTERNAL void ExecCreatePixelShader(RendererGL* renderer, PixelShaderHandle pixelShader, const DataBlob& shaderData)
{
	const char* sourcePath = nullptr;
	DEBUG_ONLY(sourcePath = shaderData.sourcePath.str);

	GLCompileResult compileResult = CompileShader(sourcePath, (GLchar*)shaderData.data, (GLint)shaderData.dataLen, GL_FRAGMENT_SHADER);
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

YU_INTERNAL void ExecReloadVertexShaderCmd(RendererGL* renderer, VertexShaderHandle vertexShader, const DataBlob& shaderData)
{
	VertexShaderGL& glVertexShader = renderer->glVertexShaderList[vertexShader.id];
	glDeleteShader(glVertexShader.vsId);
	ExecCreateVertexShader(renderer, vertexShader, shaderData);
}

YU_INTERNAL void ExecReloadPixelShaderCmd(RendererGL* renderer, PixelShaderHandle pixelShader, const DataBlob& shaderData)
{
	PixelShaderGL& glPixelShader = renderer->glPixelShaderList[pixelShader.id];
	glDeleteShader(glPixelShader.psId);
	ExecCreatePixelShader(renderer, pixelShader, shaderData);
}

YU_INTERNAL void ExecReloadPipelineCmd(RendererGL* renderer, PipelineHandle pipeline, const PipelineData& pipelineData)
{
	PipelineGL& glPipeline = renderer->glPipelineList[pipeline.id];
	glDeleteProgram(glPipeline.programId);
	ExecCreatePipeline(renderer, pipeline, pipelineData);
}

YU_INTERNAL void ExecRenderCmd(RendererGL* renderer, RenderQueue* queue, int renderListIdx)
{
	RenderCmdList* list = &(queue->renderList[renderListIdx]);
	assert(list->renderInProgress.load(std::memory_order_acquire));

	for (int i = 0; i < list->cmdCount; i++)
	{
		RenderCmd& cmd = list->cmd[i];
		MeshHandle mesh = cmd.mesh;
		CameraHandle camera = cmd.cam;
		PipelineHandle pipeline = cmd.pipeline;
		RenderResource& resources = cmd.resources;
		RenderTextureHandle renderTexture = cmd.renderTexture;
		if(renderTexture.id != renderer->currentRenderTexture.id)
		{
			RenderTextureGL& glRenderTexture = renderer->glRenderTextureList[renderTexture.id];
			glBindFramebuffer(GL_FRAMEBUFFER, glRenderTexture.fboId);
			if(glRenderTexture.dirty)
			{
				TextureHandle refTexture = renderer->renderTextureDescList[renderTexture.id].refTexture;
				int mipLevel = renderer->renderTextureDescList[renderTexture.id].mipLevel;
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer->glTextureList[refTexture.id].texId, mipLevel);
				glRenderTexture.dirty = false;
			}
			glClearColor(1, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT);
			renderer->currentRenderTexture = renderTexture;
		}
		for(unsigned int tex = 0; tex < resources.numPsTexture; tex++)
		{
			glActiveTexture(GL_TEXTURE0 + (unsigned int)tex);
			glBindTexture(GL_TEXTURE_2D, renderer->glTextureList[resources.psTextures[tex].textures.id].texId);
		}
		
		PipelineGL& glPipeline = renderer->glPipelineList[pipeline.id];
		CameraDataGL& glCamera = renderer->glCameraList[camera.id];
		MeshRenderData& meshRenderData = (renderer->meshList[mesh.id]);
		MeshDataGL& glMeshData = renderer->glMeshList[mesh.id];
		
		glBindBufferBase(GL_UNIFORM_BUFFER, glPipeline.cameraUBOIndex, glCamera.uniformBufferId);
		glUseProgram(glPipeline.programId);
		
		glBindVertexArray(glMeshData.vaoId);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glMeshData.iboId);
		
		glDrawElements(GL_TRIANGLES, (GLsizei)meshRenderData.numIndices, GL_UNSIGNED_INT, 0);
	}
	
	list->cmdCount = 0;
	list->scratchBuffer.Rewind(0);
	list->renderInProgress.store(false, std::memory_order_release);
}

YU_GLOBAL std::atomic<int> gRenderThreadRunningState; //0: stop, 1: running, 2: exiting

bool RenderThreadRunning()
{
	return gRenderThreadRunningState != 0;
}
	
YU_INTERNAL bool ExecThreadCmd(RendererGL* renderer, Allocator* allocator)
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
				case (RenderThreadCmd::STOP_RENDER_THREAD) :
				{
					gRenderThreadRunningState = 2;
				}break;
				case (RenderThreadCmd::UPDATE_CAMERA):
				{
					ExecUpdateCameraCmd(renderer, cmd.updateCameraCmd.camera, cmd.updateCameraCmd.camData);
				}break;
				case (RenderThreadCmd::CREATE_MESH):
				{
					ExecCreateMeshCmd(renderer, cmd.createMeshCmd.mesh, cmd.createMeshCmd.numVertices,
									  cmd.createMeshCmd.numIndices, cmd.createMeshCmd.vertChannelMask, cmd.createMeshCmd.meshData, allocator);
				}break;
				case (RenderThreadCmd::CREATE_TEXTURE) :
				{
					ExecCreateTextureCmd(renderer, cmd.createTextureCmd.texture, cmd.createTextureCmd.desc, cmd.createTextureCmd.initData);
				}break;
				case (RenderThreadCmd::CREATE_RENDER_TEXTURE):
				{
					ExecCreateRenderTexture(renderer, cmd.createRenderTextureCmd.renderTexture, cmd.createRenderTextureCmd.desc);
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
				#if defined (YU_DEBUG) || defined (YU_TOOL)
				case(RenderThreadCmd::RELOAD_VERTEX_SHADER):
				{
					ExecReloadVertexShaderCmd(renderer, cmd.createVSCmd.vertexShader, cmd.createVSCmd.data);
				}break;
				case(RenderThreadCmd::RELOAD_PIXEL_SHADER):
				{
					ExecReloadPixelShaderCmd(renderer, cmd.createPSCmd.pixelShader, cmd.createPSCmd.data);
				}break;
				case(RenderThreadCmd::RELOAD_PIPELINE):
				{
					ExecReloadPipelineCmd(renderer, cmd.createPipelineCmd.pipeline, cmd.createPipelineCmd.data);
				}break;
				#endif
				case(RenderThreadCmd::RENDER) :
				{
					ExecRenderCmd(renderer, queue, cmd.renderCmd.renderListIndex);
				}break;			
				case (RenderThreadCmd::SWAP) :
				{
					frameEnd = true;
				}break;
				case(RenderThreadCmd::CREATE_FENCE):
				{
					
				}break;
				case(RenderThreadCmd::INSERT_FENCE):
				{
					Fence& fence = renderer->fenceList[cmd.insertFenceCmd.fence.id];
					fence.cpuExecuted = true;
				}break;

				case(RenderThreadCmd::RESIZE_BUFFER):
				case(RenderThreadCmd::START_VR_MODE):
				case(RenderThreadCmd::STOP_VR_MODE):
				{
					Log("RenderThread ExecThreadCmd: warning, cmd is not implemented\n");
				}
			}
		}
	}
	return frameEnd;
}


//TODO: correctly handle resize buffer!!!
void ResizeBackBuffer(unsigned int width, unsigned int height, TextureFormat fmt)
{

	RenderThreadCmd cmd;
	cmd.type = RenderThreadCmd::RESIZE_BUFFER;
	cmd.resizeBuffCmd.width = width;
	cmd.resizeBuffCmd.height = height;
	cmd.resizeBuffCmd.fmt = fmt;

}

ThreadReturn ThreadCall RenderThread(ThreadContext context)
{
	InitGLParams* param = (InitGLParams*) context;
	Allocator* allocator = param->allocator;
	RendererDesc rendererDesc;
	param->initGLCS.Lock();
	Window win = param->win;
	rendererDesc = param->desc;
	rendererDesc.supportOvrRendering = false;
	InitGLContext(param->win, rendererDesc);

	gRenderer = DeepNew<RendererGL>(allocator);
	gRenderer->rendererDesc = rendererDesc;
	{
		gRenderer->frameBuffer.id = gRenderer->renderTextureIdList.Alloc();
		gRenderer->renderTextureDescList[0].refTexture.id = 0;
		gRenderer->renderTextureDescList[0].mipLevel = 0;
		gRenderer->currentRenderTexture = gRenderer->frameBuffer;
	}

	gRenderThreadRunningState = 1;
	NotifyCondVar(param->initGLCV);
	param->initGLCS.Unlock();

	FrameLock* lock = AddFrameLock();
	
	/*
	GLuint globalVao;
	glGenVertexArrays(1, &globalVao);
	glBindVertexArray(globalVao);
	*/
	
	while (gRenderThreadRunningState== 1 )
	{
		WaitForKick(lock);
		glClearColor(1, 0, 0, 1);
		glViewport(0, 0, rendererDesc.width, rendererDesc.height);
		glClear(GL_COLOR_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		
		while (!ExecThreadCmd(gRenderer, param->allocator) && gRenderThreadRunningState == 1)
			;

		BaseDoubleBufferData::SwapDirty();

		SwapBuffer(win);
		FrameComplete(lock);
	}
	

	Delete(allocator, gRenderer);

	gRenderThreadRunningState = 0;
	Log("RenderThread exit\n");

	return 0;
}

Thread	renderThread;
void InitRenderThread(const Window& win, const RendererDesc& desc, Allocator* allocator)
{
	InitGLParams param;
	param.initGLCS.Lock();
	param.win = win;
	param.desc = desc;
	param.allocator = allocator;
	renderThread = CreateThread(RenderThread, &param);
	CPUInfo cpuInfo = SystemInfo::GetCPUInfo();
	SetThreadAffinity(renderThread.handle, 1 << (cpuInfo.numLogicalProcessors - 1));
	WaitForCondVar(param.initGLCV, param.initGLCS);
	param.initGLCS.Unlock();
}


}

#endif
