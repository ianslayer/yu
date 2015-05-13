#include "system.h"
#include "thread.h"
#include "log.h"

#if defined YU_OS_WIN32
	#include <intrin.h>
#elif defined YU_OS_MAC
	//#include <sys/types.h>
	//#include <sys/sysctl.h>
#endif

namespace yu
{

struct CreateWinParam
{
	Rect	rect;
	Window	win;
	CondVar	winCreationCV;
	Mutex	winCreationCS;
};

struct CloseWinParam
{
	Window	win;
	CondVar	winCloseCV;
	Mutex	winCloseCS;
};

struct WindowThreadCmd
{
	enum CommandType
	{
		CREATE_WINDOW,
		CLOSE_WINDOW,
	};

	union
	{
		CreateWinParam*	createWinParam;
		CloseWinParam* closeWinParam;
	};

	CommandType	type;
};


using InputQueue = SpscFifo < InputEvent, 256 > ;

struct EventQueue
{
	SpscFifo<InputEvent, 256> fifo;
};

struct WindowManagerImpl
{
	WindowManagerImpl(Allocator* allocator) : windowList(allocator)
	{
		for(int i = 0; i < 16; i++)
		{
			eventQueueList[i] = nullptr;
		}
	}
		
	SpscFifo<WindowThreadCmd, 16>		winThreadCmdQueue; //yumain to window thread
	std::atomic<EventQueue*>			eventQueueList[16];
	IndexFreeList<16>					eventQueueFreeIdList;
	Array<Window>						windowList;
	Thread								windowThread;
};

void EnqueueEvent(WindowManager* mgr, InputEvent& ev)
{
	ev.timeStamp = SampleTime().time;
	for(int i = 0; i < 16; i++)
	{
		EventQueue* queue = mgr->mgrImpl->eventQueueList[i].load(std::memory_order_acquire);
		if(queue)
			queue->fifo.Enqueue(ev);
	}
}

EventQueue* CreateEventQueue(WindowManager* winMgr, Allocator* allocator)
{
	int newQueueId = winMgr->mgrImpl->eventQueueFreeIdList.Alloc();
	if(newQueueId < 0)
		return nullptr;

	if(winMgr->mgrImpl->eventQueueList[newQueueId] == nullptr)
	{
		EventQueue* queue = New<EventQueue>(allocator);
		winMgr->mgrImpl->eventQueueList[newQueueId].store(queue, std::memory_order_release);
	}

	return winMgr->mgrImpl->eventQueueList[newQueueId];
}

bool DequeueEvent(EventQueue* queue, InputEvent& ev)
{
	if(queue)
		return queue->fifo.Dequeue(ev);

	return false;
}

void InitPlatformWindowMgr(WindowManager* mgr);

WindowManager* InitWindowManager(Allocator* allocator)
{
	WindowManager* windowManager = New<WindowManager>(allocator);
	windowManager->mgrImpl = DeepNew<WindowManagerImpl>(allocator);
	
	InitPlatformWindowMgr(windowManager);
	
	Display mainDisplay = SystemInfo::GetMainDisplay();
	
	Log("main display modes:\n");
	
	int numDisplayMode = SystemInfo::NumDisplayMode(mainDisplay);
	
	for(int i = 0; i < numDisplayMode; i++)
	{
		DisplayMode mode = SystemInfo::GetDisplayMode(mainDisplay, i);
		Log("width: %lu, height: %lu, refresh rate: %lf\n", mode.width, mode.height, mode.refreshRate);
	}
	
	DisplayMode currentDisplayMode = SystemInfo::GetCurrentDisplayMode(mainDisplay);
	Log("current mode width: %lu, height: %lu, refresh rate: %lf\n", currentDisplayMode.width, currentDisplayMode.height, currentDisplayMode.refreshRate);
	
	int numDisplay = SystemInfo::NumDisplays();
	
	Log("display num: %d\n", numDisplay);
	

	CPUInfo cpuInfo = SystemInfo::GetCPUInfo();

	Log("CPU info: \n");
	Log("Vender: %s\n", cpuInfo.vender);
	

	return windowManager;
}

void FreeWindowManager(WindowManager* mgr, Allocator* allocator)
{
	Delete(allocator, mgr->mgrImpl);
	Delete(allocator, mgr);
}

#if defined (YU_CPU_X86) || defined (YU_CPU_X86_64)

void cpuid(u32 info[4], u32 cmdEax, u32 cmdEcx = 0)
{
#if defined YU_OS_WIN32
	i32* infoI = (i32*)info;
	__cpuidex(infoI, cmdEax, cmdEcx);
#elif defined YU_OS_MAC
	u32 eax, ebx, ecx, edx;
	__asm__ ("mov %4, %%eax;"
			 "mov %5, %%ecx;"
			 "cpuid;"
			 "mov %%eax, %0;"
			 "mov %%ebx, %1;"
			 "mov %%ecx, %2;"
			 "mov %%edx, %3;"
	: "=m"(eax), "=m"(ebx), "=m"(ecx), "=m"(edx)
	: "m"(cmdEax), "m"(cmdEcx)
	: "%eax", "%ebx", "ecx", "%edx"
	);
	info[0] = eax; info[1] = ebx; info[2] = ecx; info[3] = edx;
#endif
}

CPUInfo SystemInfo::GetCPUInfo()
{
/*
	int mib[2];
	size_t len=1024;
	char p[1024];

	mib[0] = CTL_HW;
	mib[1] = HW_PAGESIZE;
	//sysctl(mib, 2, p, &len, NULL, 0);
	sysctlbyname("hw.physicalcpu_max", p, &len, NULL, 0);
	*/

	CPUInfo cpuInfo;
	memset(&cpuInfo, 0, sizeof(cpuInfo));
	struct Registers
	{
		u32 eax, ebx, ecx, edx;
	};
	struct Str
	{
		char str[4];
	};
	union
	{
		Registers	reg;
		Str			str[4];
		u32			info[4];
	} info;

	u32 maxCmd;
	cpuid(info.info, 0);
	maxCmd = info.reg.eax;
	memcpy(cpuInfo.vender, &info.reg.ebx, sizeof(int));
	memcpy(cpuInfo.vender+4, &info.reg.edx, sizeof(int));
	memcpy(cpuInfo.vender+8, &info.reg.ecx, sizeof(int));

	cpuid(info.info, 1);
	u32 ecx = info.reg.ecx;
	u32 edx = info.reg.edx;

	if(ecx & (1<<28))
		cpuInfo.avx = true;
	if(ecx & (1 << 21))
		cpuInfo.x2apic = true;
	if(ecx & (1 << 20))
		cpuInfo.sse4_2 = true;
	if(ecx & (1 << 19))
		cpuInfo.sse4_1 = true;
	if(ecx & (1 << 9))
		cpuInfo.ssse3 = true;
	if( ecx & 1)
		cpuInfo.sse3 = true;
	if(edx & (1 << 26))
		cpuInfo.sse2 = true;
	if(edx & (1 <<25))
		cpuInfo.sse = true;
	if(edx & (1<<4))
		cpuInfo.tsc = true;
	
	if(maxCmd >= 0x7)
	{
		cpuid(info.info, 0x7);
		if(info.reg.ebx & (1<<5))
			cpuInfo.avx2 = true;
		
		cpuid(info.info, 0x80000007);
		if(info.reg.edx & (1<<8))
			cpuInfo.invariantTsc = true;
	}
	
	if(strcmp("GenuineIntel", cpuInfo.vender) == 0 && maxCmd >= 0xb)
	{
		u32 nextLevel = 0;
		u32 levelType = 0;
		do
		{
			cpuid(info.info, 0xb, nextLevel);
	
			levelType = (info.reg.ecx & 0xFF00) >> 8;
			u32 numProcessor = info.reg.ebx & 0xFFFF;
			
			//0 : invalid
			//1 : SMT
			//2 : Core
			//3-255 : Reserved
			if(levelType == 1)
			{
				cpuInfo.numLogicalProcessorsPerCore = numProcessor;
			}
			else if(levelType == 2)
			{
				cpuInfo.numLogicalProcessors = numProcessor;
			}
			nextLevel ++;//= (info.reg.eax & 0xF);
			
		}while(levelType != 0);
		
	}
	
	return cpuInfo;
}

#endif

}
