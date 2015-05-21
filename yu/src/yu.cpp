#include "core/system.h"
#include "core/timer.h"
#include <new>
#include "core/allocator.h"
#include "core/thread.h"
#include "core/worker.h"

#include "renderer/renderer.h"
#include "sound/sound.h"
#include "stargazer/stargazer.h"

#include "yu.h"

#include "module/module.h"
#include "core/log.h"
#include "core/file.h"
YU_GLOBAL YuCore gYuCore;


#define FR(func, proto) extern "C" proto(func) { return yu::func proto##_PARAM; }
#define F(func, proto) extern "C" proto(func) { yu::func proto##_PARAM; }
#define FV(func, proto) extern "C" proto(func) {va_list args; va_start(args, fmt); yu::V##func (fmt, args); va_end(args);}
	#include "module/module_interface.h"
#undef FV
#undef FR
#undef F

#if defined YU_OS_MAC
	#include <dlfcn.h>
	#include <sys/stat.h>
#elif defined YU_OS_WIN32
	#include <windows.h>
#endif

#include <atomic>


namespace yu
{

bool YuRunning();
void KickStart();
void WaitFrameComplete();
void FakeKickStart();

YU_GLOBAL std::atomic<int> gYuRunningState; //0: stop, 1: running, 2: exit

int YuState()
{
   	return gYuRunningState;
}

YU_GLOBAL RenderQueue*	gRenderQueue; //for shutdown render thread

struct LoadModuleResult
{
	Module* module;
	ModuleUpdateFunc* updateFunc;
};

#if defined YU_OS_MAC

bool TimeEqual(struct timespec time0, struct timespec time1)
{
	return (time0.tv_sec == time1.tv_sec) && (time0.tv_nsec == time1.tv_nsec);
}

#endif

void LoadModule(LoadModuleResult* result, WindowManager* winMgr, SysAllocator* sysAllocator)
{
	gYuCore.windowManager = winMgr;
	gYuCore.sysAllocator = sysAllocator;

#define F(func, proto) gYuCore.func = func;	
#include "module/module_interface.h"
#undef F
	
	const char* workingDir = WorkingDir();
	const char* exePath = ExePath();

	char modulePath[1024];
	size_t exeDirLength = GetDirFromPath(exePath, modulePath, sizeof(modulePath));

#if defined YU_OS_MAC
	StringBuilder modulePathBuilder(modulePath, 1024, exeDirLength);
	modulePathBuilder.Cat("test_module.dylib");

	struct stat fileState;
	YU_LOCAL_PERSIST struct timespec moduleLastModifiedTime;

	
	if(!lstat(modulePath, &fileState))
	{
		if(!TimeEqual(fileState.st_mtimespec, moduleLastModifiedTime))
		{
		   	if(result->module->moduleCode)
		   	{
				dlclose(result->module->moduleCode);
			}
			void* module = dlopen(modulePath, RTLD_NOW);
			moduleLastModifiedTime = fileState.st_mtimespec;
			if(module && result->module)
			{
				result->module->moduleCode = module;
				
				ModuleUpdateFunc* updateFunc = *(ModuleUpdateFunc*) dlsym(module, "ModuleUpdate");
				if(updateFunc)
				{
					result->updateFunc = updateFunc;
					Log("module reloaded\n");
				}
				else
				{
					result->updateFunc = nullptr;
				}
			}
			else
			{
				Log("LoadModule: error, failed to open dynamic library\n");
			}
		}
	}
	else
	{
		Log("LoadModule: error, unable to determined module file status\n");
	}
   
	
#endif
	
#if defined YU_OS_WIN32	
	
	HMODULE module = LoadLibraryA("yu/build/test_module.dll");
	
	ModuleUpdateFunc* updateFunc = (ModuleUpdateFunc*) GetProcAddress(module, "ModuleUpdate");
	updateFunc();
	if(!updateFunc)
	{
		Log("error, failed to load module\n");
	}

 #endif
}

void SetYuExit()
{
	gYuRunningState = 2;
}
	
int YuMain()
{
	InitSysLog();
	
	InitSysTime();
	SysAllocator sysAllocator = InitSysAllocator();
	InitSysStrTable(sysAllocator.sysAllocator);
	InitThreadRuntime(sysAllocator.sysAllocator);

	WindowManager* windowManager = InitWindowManager(sysAllocator.sysAllocator);

	LoadModuleResult loadedModule;
	loadedModule.module = New<Module>(sysAllocator.sysArena);
	loadedModule.module->initialized = false;
	loadedModule.module->moduleCode = nullptr;
	
	gYuRunningState = YU_RUNNING;
	
	Rect rect;
	rect.x = 128;
	rect.y = 128;
	rect.width = 1280;
	rect.height = 720;

	windowManager->mainWindow =windowManager->CreateWin(rect);
	
	RendererDesc rendererDesc = {};
	rendererDesc.frameBufferFormat = TEX_FORMAT_R8G8B8A8_UNORM;
	rendererDesc.fullScreen = false;
	rendererDesc.width = (int)rect.width;
	rendererDesc.height = (int)rect.height;
	rendererDesc.refreshRate = 60;
	rendererDesc.sampleCount = 1;
	rendererDesc.initOvrRendering = true;
	//InitSound();
	InitRenderThread(windowManager->mainWindow, rendererDesc, sysAllocator.sysAllocator);

	InitWorkerSystem(sysAllocator.sysAllocator);

	InitStarGazer(windowManager, sysAllocator.sysAllocator);

	Renderer* renderer = GetRenderer();
	gRenderQueue = GetThreadLocalRenderQueue();;
	

	unsigned int lap = 100;
	unsigned int f = 0;
	double kickStartTime = 0;
	double waitFrameTime = 0;


	while( gYuRunningState == 1 )
	{
		yu::PerfTimer frameTimer;
		yu::PerfTimer timer;
		
		frameTimer.Start();
		
		timer.Start();

		yu::KickStart();
		timer.Stop();
		kickStartTime = timer.DurationInMs();

		LoadModule(&loadedModule, windowManager, &sysAllocator);
		if(loadedModule.updateFunc)
			loadedModule.updateFunc(&gYuCore, loadedModule.module);
		
		Clear(gStarGazer);
		SubmitWork(gStarGazer);

		MainThreadWorker();

		timer.Start();
		yu::WaitFrameComplete();
		timer.Stop();
		waitFrameTime = timer.DurationInMs();
		frameTimer.Stop();

		
		f++;
		/*
		if (f > lap || frameTimer.DurationInMs() > 20)
		{
			yu::Log("main thread frame:\n");
			yu::Log("frame time: %lf\n", frameTimer.DurationInMs());
			yu::Log("kick time: %lf\n", kickStartTime);
			yu::Log("wait frame time: %lf\n", waitFrameTime);

			yu::Log("\n\n");
			
			f = 0;
		}
		*/
		
	}



	//gWindowManager->CloseWin(gWindowManager->mainWindow);

	FreeStarGazer(sysAllocator.sysAllocator);

	StopRenderThread(gRenderQueue);

	while (!AllThreadsExited() || RenderThreadRunning())
	{
		SubmitTerminateWork();
		FakeKickStart();//make sure all thread proceed to exit
	}


	FreeWindowManager(windowManager, sysAllocator.sysAllocator);
	FreeWorkerSystem(sysAllocator.sysAllocator);

	FreeThreadRuntime(sysAllocator.sysAllocator);
	FreeSysStrTable();
	FreeSysAllocator();
	FreeSysLog();

	gYuRunningState = 0;

	return 0;
}



}
