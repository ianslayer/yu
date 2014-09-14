#ifndef YU_TIMER_H
#define YU_TIMER_H

#include "platform.h"
#include "type.h"
#if defined YU_OS_WIN32
	#include <intrin.h>
#endif

namespace yu
{

class CycleCount
{
public:
	CycleCount() {}
	void Sample();
	u64 cycle;
};

class PerfTimer
{
public:
	void Start();
	void Finish();

	const CycleCount& Duration() const;
	f64		          DurationInMs() const;
	
	CycleCount        cycleCounter;
};
	
class Time
{
public:
	Time() {}
	void Sample();
	
	i64 time;
};
	
class SysTimer
{
public:
	void	Start();
	void	Finish();
	
	const Time& Duration() const;
	f64		DurationInMs() const;
	
	Time	time;
	
};

void	InitSysTime();
u64		EstimateCPUFrequency();
f64		ConvertToMs(const CycleCount& cycles);
f64		ConvertToMs(const Time& time);
f64		DurationInMs(const Time& finish, const Time& start);
	
YU_INLINE void CycleCount::Sample()
{

#if defined(YU_OS_WIN32)		
	cycle = (u64) __rdtsc();

#elif defined YU_OS_MAC && defined (__x86_64__)

	unsigned long* pSample = (unsigned long *)&cycle;

	__asm__ ("rdtsc\n\t"
			 "movl %%eax, (%0)\n\t"
			 "movl %%edx, 4(%0)\n\t"
			 :
			 : "r"(pSample)
			 : "%eax", "%edx"
			 );
	/*
	__asm
	{
		mov		rcx, pSample			
			rdtsc
			mov		[rcx], eax
			mov		[rcx + 4], edx
	}*/

#endif			
}


YU_INLINE void PerfTimer::Start()
{
	cycleCounter.Sample();
}

YU_INLINE void PerfTimer::Finish()
{
	CycleCount finishCycle;
	finishCycle.Sample();
	cycleCounter.cycle = finishCycle.cycle - cycleCounter.cycle;
}


}

#endif