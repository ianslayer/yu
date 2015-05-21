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


struct Module
{
	void* moduleCode;
	void* moduleData;
	bool initialized;
};

struct YuCore
{
	yu::WindowManager*		windowManager;
	yu::SysAllocator*		sysAllocator;

#define F(func, macro) func##Func* func;
#include "module_interface.h"
#undef F

};


#define MODULE_UPDATE(name) EXPORT void name(YuCore* core, Module* context)
typedef MODULE_UPDATE(ModuleUpdateFunc);

#endif
