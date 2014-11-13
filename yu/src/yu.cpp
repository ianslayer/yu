#include "core/system.h"
#include "math/vector.h"
#include "math/matrix.h"

#include "container/array.h"
#include "container/hash.h"

#include "core/timer.h"

#include "renderer/renderer.h"

namespace yu
{
	
void InitYu()
{
	InitSysTime();
	InitDefaultAllocator();
	InitSystem();
	
	Rect rect;
	rect.x = 128;
	rect.y = 128;
	rect.width = 1920;
	rect.height = 1080;
	
	gSystem->mainWindow = gSystem->CreateWin(rect);

	FrameBufferDesc desc;
	desc.format = TEX_FORMAT_R8G8B8A8_UNORM;
	desc.fullScreen = false;
	desc.width = (int) rect.width;
	desc.height =(int) rect.height;
	desc.sampleCount = 1;
	desc.refreshRate = 60;

	InitDX11(gSystem->mainWindow, desc);
}

void FreeYu()
{
	FreeSystem();
	FreeDefaultAllocator();
}
	
}
