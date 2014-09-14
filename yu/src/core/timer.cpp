#include "timer.h"
#include "thread.h"
#if defined YU_OS_WIN32
	#pragma comment(lib, "Winmm.lib")
#elif defined YU_OS_MAC
	#include <mach/mach_time.h>
#endif

namespace yu
{
static CycleCount initCycle;
static u64        cpuFrequency;
	
static Time	initTime;

const CycleCount& PerfTimer::Duration() const
{
	return cycleCounter;
}

f64 PerfTimer::DurationInMs() const
{
	return ConvertToMs(cycleCounter);
}
	
void Time::Sample()
{
#if defined YU_OS_WIN32

#elif defined YU_OS_MAC
	time = mach_absolute_time();
#endif
}
	
void SysTimer::Start()
{
	time.Sample();
}

void SysTimer::Finish()
{
	Time finishTime;
	finishTime.Sample();
	
	time.time = finishTime.time - time.time;
}
	
const Time& SysTimer::Duration() const
{
	return time;
}

f64 SysTimer::DurationInMs() const
{
	return ConvertToMs(time);
}
	
void InitSysTime()
{
	initCycle.Sample();
	cpuFrequency = EstimateCPUFrequency();
}

u64 EstimateCPUFrequency()
{
#if defined YU_OS_WIN32
	HANDLE currentThreadHandle = ::GetCurrentThread();
	DWORD_PTR affinity_mask = ::SetThreadAffinityMask(currentThreadHandle, 1);

	DWORD timeStartCount = timeGetTime();
	DWORD duration = 0;

	PerfTimer timer;
	timer.Start();
	while(duration < 1000)
	{
		DWORD timeCount = timeGetTime();
		duration = timeCount - timeStartCount;
	}
	timer.Finish();
	::SetThreadAffinityMask(currentThreadHandle, affinity_mask);

	return timer.Duration().cycle;
#elif defined YU_OS_MAC
	thread_act_t thread = pthread_mach_thread_np(pthread_self());
	
	mach_msg_type_number_t count = 1;
	boolean_t getDefault;
	thread_affinity_policy originAffinityTag;
	thread_policy_get(thread, THREAD_AFFINITY_POLICY, (thread_policy_t) &originAffinityTag, &count, &getDefault);
	
	thread_affinity_policy core1AffinityTag;
	core1AffinityTag.affinity_tag = 1;
	
	thread_policy_set(thread, THREAD_AFFINITY_POLICY, (thread_policy_t) &core1AffinityTag, 1);
	
	Time start, finish;
	start.Sample();
	finish.Sample();
	
	PerfTimer timer;
	timer.Start();
	
	while(DurationInMs(finish, start) < 1000)
	{
		finish.Sample();
	}
	timer.Finish();
	
	thread_policy_set(thread, THREAD_AFFINITY_POLICY, (thread_policy_t) &originAffinityTag, 1);
	
	return timer.Duration().cycle;
#endif
	return 0;
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
#elif defined YU_OS_MAC
	mach_timebase_info_data_t timeInfo;
	mach_timebase_info(&timeInfo);
	
	f64 ret = ((time.time * timeInfo.numer) / (timeInfo.denom)) / (1000000.0);
	
	return ret;
#endif
	
	return 0;
}
	
f64 DurationInMs(const Time& finish, const Time& start)
{
	Time duration;
	duration.time = finish.time - start.time;
	return ConvertToMs(duration);
}
	
}