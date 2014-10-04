#ifndef YU_THREAD_H
#define YU_THREAD_H

#include "platform.h"
#if defined YU_OS_WIN32
	#include <windows.h>
	#define COMPILER_BARRIER() _ReadWriteBarrier()
#elif defined YU_OS_MAC
	#include <mach/mach.h>
	#include <pthread.h>
	#define COMPILER_BARRIER() asm volatile("" ::: "memory")
#endif

#endif
