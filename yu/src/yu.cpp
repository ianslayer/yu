#include "yu.h"
#include "core/timer.h"
#include "core/system.h"

namespace yu
{
	
void InitYu()
{
	InitSysTime();
	InitDefaultAllocator();
	InitSystem();
}

void FreeYu()
{
	FreeSystem();
	FreeDefaultAllocator();
}
	
}
