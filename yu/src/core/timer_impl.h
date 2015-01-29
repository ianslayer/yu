#include "timer.h"
namespace yu
{
YU_GLOBAL CycleCount initCycle;
YU_GLOBAL u64        cpuFrequency;
	
YU_GLOBAL Time initTime;
YU_GLOBAL Time frameStartTime;
YU_GLOBAL u64 timerFrequency;

const CycleCount& PerfTimer::Duration() const
{
	return cycleCounter;
}

f64 PerfTimer::DurationInMs() const
{
	return ConvertToMs(cycleCounter);
}
	
Time SysStartTime()
{
	return initTime;
}
	
void Timer::Start()
{
	time = SampleTime();
}

void Timer::Stop()
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
	timer.Stop();
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