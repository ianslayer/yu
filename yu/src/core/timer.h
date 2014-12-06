#ifndef YU_TIMER_H
#define YU_TIMER_H

#include "platform.h"

#if defined YU_OS_WIN32
	#include <intrin.h>
#endif

namespace yu
{

struct CycleCount
{
	u64 cycle;
};
YU_INLINE CycleCount operator-(CycleCount c1, CycleCount c2)
{
	CycleCount c;
	c.cycle = c1.cycle - c2.cycle;
	return c;
}
CycleCount	SampleCycle();
u64			EstimateCPUFrequency();
f64			ConvertToMs(const CycleCount& cycles);

struct PerfTimer
{
	void Start();
	void Finish();

	const CycleCount& Duration() const;
	f64		          DurationInMs() const;
	
	CycleCount        cycleCounter;
};
	
struct Time
{
	u64 time;
};
YU_INLINE Time operator-(Time t1, Time t2)
{
	Time t;
	t.time = t1.time - t2.time;
	return t;
}
Time SampleTime();
Time SysStartTime();
	
class Timer
{
public:
	void	Start();
	void	Finish();
	
	const Time& Duration() const;
	f64		DurationInMs() const;
	
	Time	time;
	
};
f64		ConvertToMs(const Time& time);

void	InitSysTime();

	
YU_INLINE CycleCount SampleCycle()
{
	CycleCount count;
#if defined(YU_OS_WIN32)		
	count.cycle = (u64) __rdtsc();

#elif defined YU_OS_MAC && defined (YU_CPU_X86_64)

	unsigned long* pSample = (unsigned long *)&(count.cycle);

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
	return count;
}

YU_INLINE void PerfTimer::Start()
{
	COMPILER_BARRIER();
	cycleCounter = SampleCycle();
}

YU_INLINE void PerfTimer::Finish()
{
	CycleCount finishCycle;
	finishCycle = SampleCycle();
	COMPILER_BARRIER();
	cycleCounter = finishCycle - cycleCounter;
}

void DummyWorkLoad(double timeInMs);

}

#endif