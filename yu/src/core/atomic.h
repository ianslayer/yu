#ifndef YU_ATOMIC_H
#define YU_ATOMIC_H
#include "platform.h"
#if defined YU_OS_WIN32
	#define COMPILER_BARRIER() _ReadWriteBarrier()
#elif defined YU_OS_MAC
	#define COMPILER_BARRIER() asm volatile("" ::: "memory")
#endif

namespace yu
{

}


#endif
