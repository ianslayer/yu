#ifndef MODULE_H
#define MODULE_H

#if defined YU_CC_MSVC
	#define EXPORT
#elif defined YU_CC_CLANG
//	#define EXPORT __attribute__((visibility("default"))) // ??
	#define EXPORT
#endif

#define F(func, proto) typedef proto(func##Func);
#include "module_interface.h"
#undef F

struct FrameLockData :public yu::CompleteData
{
	yu::FrameLock* frameLock;
};

#define MAX_END_WORK_DEPENDENCY 16
struct YuCore
{
	yu::WindowManager*		windowManager;
	yu::Allocator*			sysAllocator;

	FrameLockData			frameLockData;
	yu::WorkHandle			startFrameLock;
	yu::WorkHandle			endFrameLock;

	yu::WorkHandle			endWorkDependency[MAX_END_WORK_DEPENDENCY];
	std::atomic<int>		numEndWorkDependency;
	
#define F(func, macro) func##Func* func;
#include "module_interface.h"
#undef F

};

#if defined IMPL_MODULE

YU_GLOBAL YuCore* gYuCore;

/*
#define F(func, proto) func##Func* func = gYuCore->func proto;
#undef F
*/
namespace yu
{
#define FR(func, proto) inline proto(func) { return gYuCore->func proto##_PARAM; }
#define F(func, proto) inline proto(func) { gYuCore->func proto##_PARAM; }
#define FV(func, proto)	inline proto(func) { va_list args; va_start(args, fmt); gYuCore->V##func V##proto##_PARAM; va_end(args); }
	#include "module_interface.h"
#undef FV	
#undef F
#undef FR	
}

#endif


struct Module;

#define MODULE_UPDATE(name) EXPORT void name(YuCore* core, Module* context)
typedef MODULE_UPDATE(ModuleUpdateFunc);

struct Module
{
#if defined YU_OS_MAC	
	void* moduleHandle;
#elif defined YU_OS_WIN32
	HMODULE moduleHandle;
	FILETIME lastWriteTime;
#endif
	ModuleUpdateFunc* updateFunc;
	void* moduleData;
	bool initialized;
	bool reloaded;
};

#endif
