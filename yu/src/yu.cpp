#include "core/system.h"
#include "core/timer.h"
#include <new>
#include "core/allocator.h"
#include "core/thread.h"
#include "core/worker.h"

#include "renderer/renderer.h"
#include "sound/sound.h"

#include "yu.h"

#include "module/module.h"
#include "core/log.h"
#include "core/file.h"
YU_GLOBAL YuCore gYuCore;


#define FR(func, proto) extern "C" proto(func) { return yu::func proto##_PARAM; }
#define F(func, proto) extern "C" proto(func) { yu::func proto##_PARAM; }
#define FV(func, proto) extern "C" proto(func) {va_list args; va_start(args, fmt); yu::V##func V##proto##_PARAM; va_end(args);}
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

void KickStart();
void WaitFrameComplete();
void FinalKickStart();

YU_GLOBAL std::atomic<int> gYuRunningState; //0: stop, 1: running, 2: exit

int YuState()
{
   	return gYuRunningState;
}

YU_GLOBAL RenderQueue*	gRenderQueue; //for shutdown render thread

#if defined YU_OS_MAC

bool TimeEqual(struct timespec time0, struct timespec time1)
{
	return (time0.tv_sec == time1.tv_sec) && (time0.tv_nsec == time1.tv_nsec);
}

#endif
	
void StartFrameLock(FrameLock* lock)
{
	WaitForKick(lock);
	RenderFrameStart(GetRenderer());
}

void EndFrameLock(FrameLock* lock)
{
	RenderFrameEnd(GetRenderer());
	FrameComplete(lock);
}

 void StartFrameLockWorkFunc(WorkHandle work)
 {
	 WorkData data = GetWorkData(work);
	 FrameLockData* lockData = (FrameLockData*) data.completeData;
	 StartFrameLock(lockData->frameLock);
 }

void EndFrameLockWorkFunc(WorkHandle work)
{	
	WorkData data = GetWorkData(work);
	FrameLockData* lock = (FrameLockData*)data.completeData;
	EndFrameLock(lock->frameLock);
}

bool WorkerFrameComplete()
{
	return IsDone(gYuCore.endFrameLock);
}

void CreateWorkerFrameLock(YuCore* core)
{
	core->frameLockData.frameLock = AddFrameLock();

	core->startFrameLock = NewWorkItem();
	core->endFrameLock = NewWorkItem();

	WorkData workData = {};
	workData.userData = nullptr;
	workData.completeData = &core->frameLockData;
	
	SetWorkFunc(core->startFrameLock, StartFrameLockWorkFunc, nullptr);
	SetWorkData(core->startFrameLock, workData);
	
	SetWorkFunc(core->endFrameLock, EndFrameLockWorkFunc, nullptr);
	SetWorkData(core->endFrameLock, workData);
}


void AddEndFrameDependency(YuCore* core, WorkHandle work)
{
	int allocItemIndex = core->numEndWorkDependency.fetch_add(1, std::memory_order_acquire);
	if(allocItemIndex >= MAX_END_WORK_DEPENDENCY)
	{
		Log("error: AddEndRameDependency: exceeded max dependency\n");
		assert(0);
	}
	core->endWorkDependency[allocItemIndex] = work;
}
	
//temp hack to allow stargazer junk to work
void AddEndFrameDependency(WorkHandle work)
{
	AddEndFrameDependency(&gYuCore, work);	
}
	
void LoadModule(Module* module, WindowManager* winMgr, Allocator* sysAllocator)
{
	module->reloaded = false;
	gYuCore.windowManager = winMgr;
	gYuCore.sysAllocator = sysAllocator;

#define F(func, proto) gYuCore.func = func;	
	#include "module/module_interface.h"
#undef F
	
	const char* exePath = ExePath();

	char modulePath[1024];
	char lockFilePath[1024];
	size_t exeDirLength = GetDirFromPath(exePath, modulePath, sizeof(modulePath));
	StringBuilder modulePathBuilder(modulePath, 1024, exeDirLength);
	StringBuilder lockFilePathBuilder(lockFilePath, 1024);
	lockFilePathBuilder.Cat(modulePath);
	lockFilePathBuilder.Cat("lock.tmp");
#if defined YU_OS_MAC
	modulePathBuilder.Cat("test_module.dylib");

	struct stat fileState;
	YU_LOCAL_PERSIST struct timespec moduleLastModifiedTime;

	
	if(!lstat(modulePath, &fileState))
	{
		if(!TimeEqual(fileState.st_mtimespec, moduleLastModifiedTime))
		{
		   	if(module->moduleCode)
		   	{
				dlclose(module->moduleCode);
			}
			void* moduleHandle = dlopen(modulePath, RTLD_NOW);
			moduleLastModifiedTime = fileState.st_mtimespec;
			if(moduleHandle && module)
			{
			   	module->moduleHandle = moduleHandle;
				
				ModuleUpdateFunc* updateFunc = *(ModuleUpdateFunc*) dlsym(moduleHandle, "ModuleUpdate");
				if(updateFunc)
				{
					module->updateFunc = updateFunc;
					module->reloaded = true;
					Log("module reloaded\n");
				}
				else
				{
					module->updateFunc = nullptr;
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
	char tempModulePath[1024];
	StringBuilder tempModulePathBuilder(tempModulePath, 1024);
	tempModulePathBuilder.Cat(modulePath);
	tempModulePathBuilder.Cat("temp_test_module.dll");
	modulePathBuilder.Cat("test_module.dll");

	WIN32_FILE_ATTRIBUTE_DATA lockFileData;
	WIN32_FILE_ATTRIBUTE_DATA moduleFileData;

	if(!GetFileAttributesEx(lockFilePath, GetFileExInfoStandard, &lockFileData))
	{
		if(GetFileAttributesEx(modulePath, GetFileExInfoStandard, &moduleFileData))
		{
			if(CompareFileTime(&module->lastWriteTime, &moduleFileData.ftLastWriteTime) != 0)
			{
				module->lastWriteTime = moduleFileData.ftLastWriteTime;
			
				if(module->moduleHandle)
				{
					FreeLibrary(module->moduleHandle);
				}
								
				CopyFile(modulePath, tempModulePath, FALSE);

				HMODULE moduleHandle = LoadLibraryA(tempModulePath);

				if(moduleHandle && module)
				{
					module->moduleHandle = moduleHandle;
					ModuleUpdateFunc* updateFunc = (ModuleUpdateFunc*) GetProcAddress(moduleHandle, "ModuleUpdate");
		
					if(updateFunc)
					{
						module->updateFunc = updateFunc;
						module->reloaded = true;
						Log("module reloaded\n");
					}
					else
					{
						module->updateFunc = nullptr;
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
	Allocator* sysAllocator = InitSysAllocator();
	InitThreadRuntime(sysAllocator);
	
	InitSysStrTable();
	WindowManager* windowManager = InitWindowManager();
	
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
	InitRenderThread(windowManager->mainWindow, rendererDesc, sysAllocator);

	InitWorkerSystem();

	Module loadedModule = {};
	CreateWorkerFrameLock(&gYuCore);

	Renderer* renderer = GetRenderer();
	gRenderQueue = GetThreadLocalRenderQueue();;
	

	unsigned int lap = 100;
	unsigned int f = 0;
	double kickStartTime = 0;
	double waitFrameTime = 0;
	
	
	while( gYuRunningState == YU_RUNNING )
	{
		yu::PerfTimer frameTimer;
		yu::PerfTimer timer;
		
		frameTimer.Start();
		
		timer.Start();

		RendererFrameCleanup(renderer);
		
		yu::KickStart();
		timer.Stop();
		kickStartTime = timer.DurationInMs();
		
		LoadModule(&loadedModule, windowManager, sysAllocator);

		gYuCore.numEndWorkDependency = 0;
		ResetWorkItem(gYuCore.startFrameLock);
		ResetWorkItem(gYuCore.endFrameLock);
		SubmitWorkItem(gYuCore.startFrameLock, nullptr, 0);
		
		if(loadedModule.updateFunc)
			loadedModule.updateFunc(&gYuCore, &loadedModule);

		SubmitWorkItem(gYuCore.endFrameLock, gYuCore.endWorkDependency, gYuCore.numEndWorkDependency);
		
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


	StopRenderThread(gRenderQueue);

	while (!AllThreadsExited() || RenderThreadRunning())
	{
		SubmitTerminateWork();
		FinalKickStart();//make sure all thread proceed to exit
	}


	FreeWindowManager(windowManager);
	FreeWorkerSystem();

	FreeThreadRuntime(sysAllocator);
	FreeSysStrTable();
	FreeSysAllocator();
	FreeSysLog();

	gYuRunningState = YU_STOPPED;

	return 0;
}



}
