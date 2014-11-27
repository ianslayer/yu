#include "core/log.h"
#include "core/system.h"
#include "math/vector.h"
#include "math/matrix.h"

#include "container/array.h"
#include "container/hash.h"

#include "core/timer.h"
#include "core/thread.h"

#include "renderer/renderer.h"

#include "yu.h"

#include <atomic>

namespace yu
{

bool YuRunning();

ThreadReturn ThreadCall DummyThreadFunc(ThreadContext context)
{
	FrameLock* lock = AddFrameLock(GetCurrentThreadHandle());

	unsigned int laps = 100;
	unsigned int f = 0;
	
	while (YuRunning())
	{
		PerfTimer frameTimer;
		PerfTimer innerTimer;
		PerfTimer kTimer;
		
		double waitKickTime = 0;
		double completeFrameTime = 0;

		frameTimer.Start();

		kTimer.Start();
		WaitForKick(lock);
		kTimer.Finish();

		waitKickTime = kTimer.DurationInMs();

		//Log("wait kick takes %lf ms\n", timer.DurationInMs());

		innerTimer.Start();
		DummyWorkLoad(12);
		innerTimer.Finish();

		kTimer.Start();
		FrameComplete(lock);
		kTimer.Finish();
		completeFrameTime = kTimer.DurationInMs();

		frameTimer.Finish();
		
		f++;
		
		if (f > laps)
		{
			Log("thread frame:\n");
			Log("frame time: %lf\n", frameTimer.DurationInMs());
			Log("work time: %lf\n", innerTimer.DurationInMs());
			Log("wait kick time: %lf\n", waitKickTime);
			Log("frame complete: %lf\n", completeFrameTime);
			Log("\n\n");
			f = 0;
		}

	}
	return 0;
}

std::atomic<int> gYuRunning;
void InitYu()
{
	InitLog();
	gYuRunning = 1;
	InitSysTime();
	InitThreadRuntime();
	std::atomic_thread_fence(std::memory_order_seq_cst);
	InitDefaultAllocator();
	InitSystem();

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
	
#if defined YU_OS_WIN32
	InitDX11Thread(gSystem->mainWindow, frameBufferDesc);
#endif
	
	//TEST create dummy thread
	Thread thread0 = CreateThread(DummyThreadFunc, nullptr);
	SetThreadAffinity(thread0.threadHandle, 1);
	//Thread thread1 = CreateThread(DummyThradFunc, nullptr);
	//SetThreadAffinity(thread1.threadHandle, 2);
	//Thread thread2 = CreateThread(DummyThradFunc, nullptr);
	//SetThreadAffinity(thread2.threadHandle, 16);
	
}
void FakeKickStart();
void FreeYu()
{
	FakeKickStart(); //make sure all thread proceed to exit
	while (!AllThreadExited());
	//OutputDebugString(TEXT("all worker thread exited\n"));
	FreeSystem();
	FreeDefaultAllocator();
	FreeThreadRuntime();
}

bool YuRunning()
{
	return (gYuRunning == 1);
}

void SetYuExit()
{
	gYuRunning.fetch_sub(1, std::memory_order_seq_cst);
}

int YuMain()
{
	yu::InitYu();

	yu::SetThreadAffinity(yu::GetCurrentThreadHandle(), 1);
	unsigned int lap = 100;
	unsigned int f = 0;
	double kickStartTime = 0;
	double waitKickTime = 0;
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
		
		timer.Start();
		
		yu::WaitFrameComplete();
		timer.Finish();
		waitFrameTime = timer.DurationInMs();
		//printf("frame time: %lf\n", timer.DurationInMs());
		frameTimer.Finish();

		
		f++;
		if (f > lap || frameTimer.DurationInMs() > 20)
		{
			yu::Log("main thread frame:\n");
			yu::Log("frame time: %lf\n", frameTimer.DurationInMs());
			yu::Log("kick time: %lf\n", kickStartTime);
			yu::Log("wait frame time: %lf\n", waitFrameTime);

			yu::Log("\n\n");
			
			f = 0;
		}
		
	}

	yu::FreeYu();

	return 0;
}



}
