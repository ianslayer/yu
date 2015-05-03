#include "core/system.h"
#include "core/timer.h"

#include "renderer/renderer.h"
#include "sound/sound.h"
#include "stargazer/stargazer.h"

#include "module/module.h"
#include "core/log.h"
#include "core/file.h"

#include "yu.h"
#include <atomic>
#include <windows.h>
namespace yu
{

bool YuRunning();
void KickStart();
void WaitFrameComplete();
void FakeKickStart();

YU_GLOBAL std::atomic<int> gYuRunning;
YU_GLOBAL std::atomic<int> gYuInitialized;
YU_GLOBAL RenderQueue*	gRenderQueue; //for shutdown render thread

void LoadModule()
{
	HMODULE module = LoadLibraryA("yu/build/test_module.dll");
	const char* workingDir = WorkingDir();
	const char* exePath = ExePath();

	char exeDir[1024];
	size_t exeDirLength = GetDirFromPath(exePath, exeDir, sizeof(exeDir));
	
	module_update* updateFunc = (module_update*) GetProcAddress(module, "ModuleUpdate");
	updateFunc();
	if(!updateFunc)
	{
		Log("error, failed to load module\n");
	}
}

void InitYu()
{
	
	InitSysLog();

	LoadModule();
	
	gYuRunning = 1;
	InitSysTime();
	InitSysAllocator();
	InitSysStrTable();
	InitThreadRuntime();

	InitWindowManager();

	gYuInitialized = 1;

	Rect rect;
	rect.x = 128;
	rect.y = 128;
	rect.width = 1280;
	rect.height = 720;

	gWindowManager->mainWindow = gWindowManager->CreateWin(rect);
	
	RendererDesc rendererDesc = {};
	rendererDesc.frameBufferFormat = TEX_FORMAT_R8G8B8A8_UNORM;
	rendererDesc.fullScreen = false;
	rendererDesc.width = (int)rect.width;
	rendererDesc.height = (int)rect.height;
	rendererDesc.refreshRate = 60;
	rendererDesc.sampleCount = 1;
	rendererDesc.initOvrRendering = true;
	//InitSound();
	InitRenderThread(gWindowManager->mainWindow, rendererDesc);

	InitWorkerSystem();

	InitStarGazer();

	Renderer* renderer = GetRenderer();
	gRenderQueue = GetThreadLocalRenderQueue();;
}

void FreeYu()
{
	SubmitTerminateWork();

	//gWindowManager->CloseWin(gWindowManager->mainWindow);

	FreeStarGazer();

	StopRenderThread(gRenderQueue);

	while (!AllThreadsExited())
	{
		FakeKickStart();//make sure all thread proceed to exit
	}


	FreeWindowManager();
	FreeWorkerSystem();

	FreeThreadRuntime();
	FreeSysStrTable();
	FreeSysAllocator();
	FreeSysLog();
}

bool Initialized()
{
	return (gYuInitialized.load(std::memory_order_acquire) == 1);
}

bool YuRunning()
{
	return (gYuRunning.load(std::memory_order_acquire) == 1);
}

void SetYuExit()
{
	gYuRunning.fetch_sub(1);
}

int YuMain()
{
	yu::InitYu();

	unsigned int lap = 100;
	unsigned int f = 0;
	double kickStartTime = 0;
	double waitFrameTime = 0;


	while( yu::YuRunning() )
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

	yu::FreeYu();

	return 0;
}



}
