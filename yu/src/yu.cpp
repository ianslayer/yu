#include "core/system.h"
#include "core/timer.h"

#include "renderer/renderer.h"
#include "sound/sound.h"
#include "stargazer/stargazer.h"

#include "yu.h"
#include <atomic>
namespace yu
{

bool YuRunning();
void KickStart();
void WaitFrameComplete();
void FakeKickStart();

std::atomic<int> gYuRunning;
std::atomic<int> gYuInitialized;
void InitYu()
{
	InitSysLog();
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
	
	FrameBufferDesc frameBufferDesc;
	frameBufferDesc.format = TEX_FORMAT_R8G8B8A8_UNORM;
	frameBufferDesc.fullScreen = false;
	frameBufferDesc.width = (int) rect.width;
	frameBufferDesc.height = (int) rect.height;
	frameBufferDesc.refreshRate = 60;
	frameBufferDesc.sampleCount = 1;
	//InitSound();
	InitRenderThread(gWindowManager->mainWindow, frameBufferDesc);


	InitWorkerSystem();


	InitStarGazer();


}

void FreeYu()
{
	SubmitTerminateWork();

	//gWindowManager->CloseWin(gWindowManager->mainWindow);

	while (!AllThreadsExited())
	{
		FakeKickStart();//make sure all thread proceed to exit
	}

	FreeStarGazer();

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
