#include "core/system.h"
#include "core/timer.h"
#include "core/allocator.h"
#include "core/thread.h"
#include "core/worker.h"

#include "renderer/renderer.h"
#include "sound/sound.h"
#include "stargazer/stargazer.h"

#include "module/module.h"
#include "core/log.h"
#include "core/file.h"

#include <atomic>
//#include <windows.h>
namespace yu
{

bool YuRunning();
void KickStart();
void WaitFrameComplete();
void FakeKickStart();

YU_GLOBAL std::atomic<int> gYuRunningState; //0: stop, 1: running, 2: exit
YU_GLOBAL std::atomic<int> gYuInitialized;
YU_GLOBAL RenderQueue*	gRenderQueue; //for shutdown render thread

void LoadModule()
{

   	const char* workingDir = WorkingDir();
	const char* exePath = ExePath();

	/*
	HMODULE module = LoadLibraryA("yu/build/test_module.dll");
	
	char exeDir[1024];
	size_t exeDirLength = GetDirFromPath(exePath, exeDir, sizeof(exeDir));
	
	module_update* updateFunc = (module_update*) GetProcAddress(module, "ModuleUpdate");
	updateFunc();
	if(!updateFunc)
	{
		Log("error, failed to load module\n");
	}
	*/
}

bool Initialized()
{
  	return (gYuInitialized.load(std::memory_order_acquire) == 1);
}

bool YuRunning()
{
	return (gYuRunningState.load(std::memory_order_acquire) == 1 );
}

bool YuStopped()
{
	return gYuRunningState == 0;
}
	
void SetYuExit()
{
	gYuRunningState = 2;
}
	
int YuMain()
{
	InitSysLog();

	LoadModule();
	
	gYuRunningState = 1;
	InitSysTime();
	SysAllocator sysAllocator = InitSysAllocator();
	InitSysStrTable(sysAllocator.sysAllocator);
	InitThreadRuntime(sysAllocator.sysAllocator);

	WindowManager* windowManager = InitWindowManager(sysAllocator.sysAllocator);

	gYuInitialized = 1;

	Rect rect;
	rect.x = 128;
	rect.y = 128;
	rect.width = 1280;
	rect.height = 720;

	windowManager->mainWindow =windowManager->CreateWin(rect);
	
	RendererDesc rendererDesc = {};
	rendererDesc.frameBufferFormat = TEX_FORMAT_R8G8B8A8_UNORM;
	rendererDesc.fullScreen = false;
	rendererDesc.width = (int)rect.width;
	rendererDesc.height = (int)rect.height;
	rendererDesc.refreshRate = 60;
	rendererDesc.sampleCount = 1;
	rendererDesc.initOvrRendering = true;
	//InitSound();
	InitRenderThread(windowManager->mainWindow, rendererDesc, sysAllocator.sysAllocator);

	InitWorkerSystem(sysAllocator.sysAllocator);

	InitStarGazer(windowManager, sysAllocator.sysAllocator);

	Renderer* renderer = GetRenderer();
	gRenderQueue = GetThreadLocalRenderQueue();;
	

	unsigned int lap = 100;
	unsigned int f = 0;
	double kickStartTime = 0;
	double waitFrameTime = 0;


	while( gYuRunningState == 1 )
	{
		yu::PerfTimer frameTimer;
		yu::PerfTimer timer;
		
		frameTimer.Start();
		
		timer.Start();

		yu::KickStart();
		timer.Stop();
		kickStartTime = timer.DurationInMs();

		Clear(gStarGazer);
		SubmitWork(gStarGazer);

		MainThreadWorker();

		timer.Start();
		yu::WaitFrameComplete();
		timer.Stop();
		waitFrameTime = timer.DurationInMs();
		frameTimer.Stop();

		
		f++;
		/*
		if (f > lap || frameTimer.DurationInMs() > 20)
		{
			yu::Log("main thread frame:\n");
			yu::Log("frame time: %lf\n", frameTimer.DurationInMs());
			yu::Log("kick time: %lf\n", kickStartTime);
			yu::Log("wait frame time: %lf\n", waitFrameTime);

			yu::Log("\n\n");
			
			f = 0;
		}
		*/
		
	}



	//gWindowManager->CloseWin(gWindowManager->mainWindow);

	FreeStarGazer(sysAllocator.sysAllocator);

	StopRenderThread(gRenderQueue);

	while (!AllThreadsExited() || RenderThreadRunning())
	{
		SubmitTerminateWork();
		FakeKickStart();//make sure all thread proceed to exit
	}


	FreeWindowManager(windowManager, sysAllocator.sysAllocator);
	FreeWorkerSystem(sysAllocator.sysAllocator);

	FreeThreadRuntime(sysAllocator.sysAllocator);
	FreeSysStrTable();
	FreeSysAllocator();
	FreeSysLog();

	gYuRunningState = 0;

	return 0;
}



}
