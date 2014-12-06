#include "timer.h"
#include "log.h"
#include "thread.h"

#if defined YU_OS_WIN32
	#include <windows.h>
	#pragma comment(lib, "Winmm.lib")
#elif defined YU_OS_MAC
	#include <mach/mach_time.h>
#endif

namespace yu
{
static CycleCount initCycle;
static u64        cpuFrequency;
	
static Time initTime;
static u64 timerFrequency;

const CycleCount& PerfTimer::Duration() const
{
	return cycleCounter;
}

f64 PerfTimer::DurationInMs() const
{
	return ConvertToMs(cycleCounter);
}
	
Time SampleTime()
{
	Time time;
#if defined YU_OS_WIN32
	LARGE_INTEGER perfCount;
	QueryPerformanceCounter(&perfCount);
	time.time = perfCount.QuadPart;
#elif defined YU_OS_MAC
	time.time = mach_absolute_time();
#endif
	return time;
}

Time SysStartTime()
{
	return initTime;
}
	
void Timer::Start()
{
	time = SampleTime();
}

void Timer::Finish()
{
	Time finishTime = SampleTime();
	time.time = finishTime.time - time.time;
}
	
const Time& Timer::Duration() const
{
	return time;
}

f64 Timer::DurationInMs() const
{
	return ConvertToMs(time);
}
	
void InitSysTime()
{
#if defined YU_OS_WIN32
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	timerFrequency = frequency.QuadPart;
#elif defined YU_OS_MAC
	mach_timebase_info_data_t timeInfo;
	mach_timebase_info(&timeInfo);
	timerFrequency = timeInfo.numer / timeInfo.denom;
#endif

	initCycle = SampleCycle();
	cpuFrequency = EstimateCPUFrequency();
	
	initTime = SampleTime();

	Log("estimated cpu freq: %llu\n", cpuFrequency);
	Log("timer freq: %llu\n", timerFrequency);
}

u64 EstimateCPUFrequency()
{
	ThreadHandle currentThreadHandle = GetCurrentThreadHandle();
	u64 originAffinity = GetThreadAffinity(currentThreadHandle);
	SetThreadAffinity(currentThreadHandle, 1);

	Time startCount = SampleTime();
	Time duration = {};

	PerfTimer timer;
	timer.Start();
	while (ConvertToMs(duration) < 1000)
	{
		Time curTime = SampleTime();
		duration = curTime - startCount;
	}
	timer.Finish();
	SetThreadAffinity(currentThreadHandle, originAffinity);

	return timer.Duration().cycle;
}

f64 ConvertToMs(const CycleCount& cycles)
{
	i64 time     = (i64)(cycles.cycle / cpuFrequency);  // unsigned->sign conversion should be safe here
	i64 timeFract = (i64)(cycles.cycle % cpuFrequency);  // unsigned->sign conversion should be safe here
	f64 ret = (time) + (f64)timeFract/(f64)((i64)cpuFrequency);
	return ret * 1000.0;
}

f64 ConvertToMs(const Time& time)
{
#if defined YU_OS_WIN32
	u64 timeInMs = time.time * 1000;
	return (f64) timeInMs /(f64)timerFrequency;
#elif defined YU_OS_MAC
	mach_timebase_info_data_t timeInfo;
	mach_timebase_info(&timeInfo);
	
	f64 ret = ((time.time * timeInfo.numer) / (timeInfo.denom)) / (1000000.0);
	
	return ret;
#endif
}


void DummyWorkLoad(double timeInMs)
{
	Time startTime = SampleTime();
	double deltaT = 0;
	while (1)
	{
		if (deltaT >= timeInMs)
			break;
		Time endTime = SampleTime();

		deltaT = ConvertToMs(endTime - startTime);
	}
}
	
}