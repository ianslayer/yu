#include "timer.h"

#ifdef YU_OS_WIN32
	//#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#pragma comment(lib, "Winmm.lib")
#endif

namespace yu
{
static u64        initializeTime;
static u64        cpuFrequency;

const CycleCount& Timer::Duration() const
{
	return cycleCounter;
}

f64 Timer::DurationInMillisecnds()
{
	return ConvertToMillisecnds(cycleCounter);
}

void InitSysTime()
{
	CycleCount counter;
	counter.Sample();
	initializeTime = counter.cycle;
	cpuFrequency = EstimateCPUFrequency();
}

u64 EstimateCPUFrequency()
{
	HANDLE currentThreadHandle = ::GetCurrentThread();
	DWORD_PTR affinity_mask = ::SetThreadAffinityMask(currentThreadHandle, 1);

	DWORD timeStartCount = timeGetTime();
	DWORD duration = 0;

	Timer timer;
	timer.Start();
	while(duration < 1000)
	{
		DWORD timeCount = timeGetTime();
		duration = timeCount - timeStartCount;
	}
	timer.Finish();
	::SetThreadAffinityMask(currentThreadHandle, affinity_mask);

	return timer.Duration().cycle;
}

f64 ConvertToMillisecnds(const CycleCount& cycles)
{
	i64 time     = (i64)(cycles.cycle / cpuFrequency);  // unsigned->sign conversion should be safe here
	i64 timeFract = (i64)(cycles.cycle % cpuFrequency);  // unsigned->sign conversion should be safe here
	f64 ret = (time) + (f64)timeFract/(f64)((i64)cpuFrequency);
	return ret * 1000.0;
}


}