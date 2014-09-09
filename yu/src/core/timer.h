#ifndef YU_TIMER_H
#define YU_TIMER_H

#include "platform.h"
#include "type.h"
#ifdef YU_OS_WIN32
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

class Timer
{
public:
	void Start();
	void Finish();

	const CycleCount& Duration() const;
	f64		          DurationInMillisecnds(); 
	
	CycleCount        cycleCounter;
};

void	InitSysTime();
u64		EstimateCPUFrequency();
f64		ConvertToMillisecnds(const CycleCount& cycles);

YU_INLINE void CycleCount::Sample()
{

#if defined(YU_OS_WIN32)		
	cycle = (u64) __rdtsc();

#elif defined YU_OS_MAC && defined (__x86_64__)

	unsigned long* pSample = (unsigned long *)&m_uint64;

	__asm
	{
		mov		rcx, pSample			
			rdtsc
			mov		[rcx], eax
			mov		[rcx + 4], edx
	}

#elif defined YU_OS_MAC

	unsigned long* pSample = (unsigned long *)&m_uint64;
	__asm
	{
		mov		ecx, pSample
			rdtsc
			mov		[ecx], eax
			mov		[ecx+4], edx
	}
#endif			
}


YU_INLINE void Timer::Start()
{
	cycleCounter.Sample();
}

YU_INLINE void Timer::Finish()
{
	CycleCount endCount;
	endCount.Sample();
	cycleCounter.cycle = endCount.cycle - cycleCounter.cycle;
}

}

#endif