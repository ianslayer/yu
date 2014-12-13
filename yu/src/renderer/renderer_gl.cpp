#include "../core/thread.h"
#include "../core/system.h"
#include "renderer_impl.h"
#include "gl_utility.h"
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
		SwapBuffer(win);
		FrameComplete(lock);
	}
	
	return 0;
}

struct RendererGL : public Renderer
{
};

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
	return new RendererGL;
}

void FreeRenderer(Renderer* renderer)
{
	RendererGL* rendererGL = (RendererGL*) renderer;
	delete rendererGL;
}

}
