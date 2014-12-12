#include "core/log.h"
#include "core/system.h"
#include "core/allocator.h"
#include "core/timer.h"
#include "core/thread.h"
#include "core/worker.h"
#include "renderer/renderer.h"
#include "app/work_map.h"

#include "yu.h"

namespace yu
{

bool YuRunning();
void KickStart();
void WaitFrameComplete();
void FakeKickStart();

std::atomic<int> gYuRunning;
void InitYu()
{
	InitSysLog();
	gYuRunning = 1;
	InitSysTime();
	InitSysAllocator();
	InitSysStrTable();
	InitThreadRuntime();
	InitWorkerSystem();
	InitSystem();

	InitWorkMap();

	Rect rect;
	rect.x = 128;
	rect.y = 128;
	rect.width = 1280;
	rect.height = 720;

	gSystem->mainWindow = gSystem->CreateWin(rect);
	
	FrameBufferDesc frameBufferDesc;
	frameBufferDesc.format = TEX_FORMAT_R8G8B8A8_UNORM;
	frameBufferDesc.fullScreen = false;
	frameBufferDesc.width = (int) rect.width;
	frameBufferDesc.height = (int) rect.height;
	frameBufferDesc.refreshRate = 60;
	frameBufferDesc.sampleCount = 1;
	
	InitRenderThread(gSystem->mainWindow, frameBufferDesc);

}

void FreeYu()
{
	SubmitTerminateWork();

	while (!AllThreadsExited())
	{
		FakeKickStart();//make sure all thread proceed to exit
	}

	FreeWorkMap();
	FreeSystem();
	FreeWorkerSystem();
	FreeThreadRuntime();
	FreeSysStrTable();
	FreeSysAllocator();
	FreeSysLog();
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
		timer.Finish();
		kickStartTime = timer.DurationInMs();

		Clear(gWorkMap);
		SubmitWork(gWorkMap);

		MainThreadWorker();

		timer.Start();
		yu::WaitFrameComplete();
		timer.Finish();
		waitFrameTime = timer.DurationInMs();
		frameTimer.Finish();

		
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
