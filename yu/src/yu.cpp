
#include "yu.h"
#include "core/system.h"
#include "math/vector.h"
#include "math/matrix.h"

#include "container/array.h"
#include "container/hash.h"

#include "core/timer.h"


namespace yu
{
	
void InitYu()
{
	InitSysTime();
	InitDefaultAllocator();
	InitSystem();
	
	Rect rect;
	rect.x = 0;
	rect.y = 0;
	rect.width = 800;
	rect.height = 600;
	
	gSystem->mainWindow = gSystem->CreateWindow(rect);
}

void FreeYu()
{
	FreeSystem();
	FreeDefaultAllocator();
}
	
}
