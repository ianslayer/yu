#include "yu.h"
#include "core/thread.h"
#include "core/timer.h"
#include <stdio.h>
#include <windows.h>

int main()
{
	yu::InitYu();

	
	SetThreadAffinityMask(GetCurrentThread(), 16);
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
		yu::DummyWorkLoad(5);
		timer.Start();
		
		yu::WaitFrameComplete();
		timer.Finish();
		waitFrameTime = timer.DurationInMs();
		//printf("frame time: %lf\n", timer.DurationInMs());
		frameTimer.Finish();

		
		f++;
		if (f > lap || frameTimer.DurationInMs() > 20)
		{
			printf("main thread frame:\n");
			printf("frame time: %lf\n", frameTimer.DurationInMs());
			printf("kick time: %lf\n", kickStartTime);
			printf("wait frame time: %lf\n", waitFrameTime);

			printf("\n\n");
			
			f = 0;
		}
		
	}

	yu::FreeYu();

	return 0;
}