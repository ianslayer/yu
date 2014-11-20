#include "core/system.h"
#include "math/vector.h"
#include "math/matrix.h"

#include "container/array.h"
#include "container/hash.h"

#include "core/timer.h"
#include "core/thread.h"

#include "renderer/renderer.h"

#include <atomic>

namespace yu
{

Event* gFrameSync;

bool YuRunning();

ThreadReturn ThreadCall DummyThradFunc(ThreadContext context)
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

		//printf("wait kick takes %lf ms\n", timer.DurationInMs());

		innerTimer.Start();
		double deltaT = 0;
		DummyWorkLoad(10);
		innerTimer.Finish();

		kTimer.Start();
		FrameComplete(lock);
		kTimer.Finish();
		completeFrameTime = kTimer.DurationInMs();

		frameTimer.Finish();
		
		
		f++;
		/*
		if (f > laps || frameTimer.DurationInMs() > 20)
		{
			printf("thread frame:\n");
			printf("frame time: %lf\n", frameTimer.DurationInMs());
			printf("work time: %lf\n", innerTimer.DurationInMs());
			printf("wait kick time: %lf\n", waitKickTime);
			printf("frame complete: %lf\n", completeFrameTime);
			printf("\n\n");
			f = 0;
		}
		*/

	}
	return 0;
}

void InitYu()
{
	InitSysTime();
	InitThreadRuntime();
	gFrameSync = new Event();
	std::atomic_thread_fence(std::memory_order_seq_cst);
	InitDefaultAllocator();
	InitSystem();

	Rect rect;
	rect.x = 128;
	rect.y = 128;
	rect.width = 1920;
	rect.height = 1080;

	gSystem->mainWindow = gSystem->CreateWin(rect);
	
	FrameBufferDesc frameBufferDesc;
	frameBufferDesc.format = TEX_FORMAT_R8G8B8A8_UNORM;
	frameBufferDesc.fullScreen = false;
	frameBufferDesc.width = (int) rect.width;
	frameBufferDesc.height = (int) rect.height;
	frameBufferDesc.refreshRate = 60;
	frameBufferDesc.sampleCount = 1;
	InitDX11Thread(gSystem->mainWindow, frameBufferDesc);
	
	
	//TEST create dummy thread
	Thread thread0 = CreateThread(DummyThradFunc, nullptr);
	SetThreadAffinity(thread0.threadHandle, 1);
	Thread thread1 = CreateThread(DummyThradFunc, nullptr);
	SetThreadAffinity(thread1.threadHandle, 4);
	//Thread thread2 = CreateThread(DummyThradFunc, nullptr);
	//SetThreadAffinity(thread2.threadHandle, 16);

	
}
void FakeKickStart();
void FreeYu()
{
	FakeKickStart(); //make sure all thread proceed to exit
	while (!AllThreadExited());
	OutputDebugString(TEXT("all worker thread exited\n"));
	delete gFrameSync;
	FreeSystem();
	FreeDefaultAllocator();
	FreeThreadRuntime();
}

std::atomic<int> gYuRunning = 1;

bool YuRunning()
{
	return (gYuRunning == 1);
}

void SetYuExit()
{
	gYuRunning.fetch_sub(1, std::memory_order_seq_cst);
}





}
