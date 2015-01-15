#include "../core/thread.h"
#include "../core/system.h"
#include "../core/allocator.h"
#include "shader.h"
#include "renderer_impl.h"
#include "gl_utility.h"
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


struct RendererGL : public Renderer
{
};

RendererGL* gRenderer;

static void ExecRenderCmd(RendererGL* renderer, RenderQueue* queue, int renderListIdx)
{
	RenderList* list = &(queue->renderList[renderListIdx]);
	assert(list->renderInProgress.load(std::memory_order_acquire));


	for (int i = 0; i < list->cmdCount; i++)
	{
	}
	
	list->cmdCount = 0;
	list->renderInProgress.store(false, std::memory_order_release);
}

static bool ExecThreadCmd(RendererGL* renderer)
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
	param->initGLCS.Lock();
	Window win = param->win;
	InitGLContext(param->win, param->desc);
	
	NotifyCondVar(param->initGLCV);
	param->initGLCS.Unlock();

	FrameLock* lock = AddFrameLock();
	while(YuRunning())
	{
		WaitForKick(lock);
		glClearColor(1, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);
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
	CPUInfo cpuInfo = System::GetCPUInfo();
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
