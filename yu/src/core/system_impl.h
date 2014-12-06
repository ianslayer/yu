#include "system.h"
#include "timer.h"
#include "thread.h"
#include "string.h"
#include "log.h"
#include "../container/dequeue.h"
#include "../container/array.h"

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


struct WindowThreadCmd
{
	enum CommandType
	{
		CREATE_WINDOW,
	};

	union Command
	{
		CreateWinParam*	createWinParam;
	};

	CommandType	type;
	Command		cmd;
};

class SystemImpl : public System
{
public:
	SpscFifo<WindowThreadCmd, 16>		winThreadCmdQueue; //yumain to window thread
	SpscFifo<InputEvent, 256>			inputQueue;			//window to yumain thread
	Array<Window>						windowList;
	Thread								windowThread;
};

System* gSystem = 0;

bool PlatformInitSystem();

bool InitSystem()
{
	gSystem = new System();

	if(!PlatformInitSystem())
	{
		return false;
	}
	
	Display mainDisplay = System::GetMainDisplay();
	
	Log("main display modes:\n");
	
	int numDisplayMode = System::NumDisplayMode(mainDisplay);
	
	for(int i = 0; i < numDisplayMode; i++)
	{
		DisplayMode mode = System::GetDisplayMode(mainDisplay, i);
		Log("width: %lu, height: %lu, refresh rate: %lf\n", mode.width, mode.height, mode.refreshRate);
	}
	
	DisplayMode currentDisplayMode = System::GetCurrentDisplayMode(mainDisplay);
	Log("current mode width: %lu, height: %lu, refresh rate: %lf\n", currentDisplayMode.width, currentDisplayMode.height, currentDisplayMode.refreshRate);
	
	int numDisplay = System::NumDisplays();
	
	Log("display num: %d\n", numDisplay);
	

	CPUInfo cpuInfo = System::GetCPUInfo();

	Log("CPU info: \n");
	Log("Vender: %s\n", cpuInfo.vender);
	

	return true;
}

void FreeSystem()
{

	delete gSystem;
	gSystem = 0;
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
	: "=r"(eax), "=r"(ebx), "=r"(ecx), "=r"(edx)
	: "r"(cmdEax), "r"(cmdEcx)
	: "%eax", "%ebx", "ecx", "%edx"
	);
	info[0] = eax; info[1] = ebx; info[2] = ecx; info[3] = edx;
#endif
}

CPUInfo System::GetCPUInfo()
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
	union Info
	{
		Registers	reg;
		Str			str[4];
		u32			info[4];
	};

	Info info;
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

System::~System()
{ 
	delete sysImpl;
}

void System::ProcessInput()
{
	InputEvent event;
	while (gSystem->sysImpl->inputQueue.Dequeue(event))
	{
		Time sysInitTime = SysStartTime();
		Time eventTime;
		eventTime.time = event.timeStamp;


		switch (event.type)
		{
			case InputEvent::KEYBOARD:
			{
				Log("event time: %f ", ConvertToMs(eventTime - sysInitTime) / 1000.f);
				if (event.data.keyboardEvent.type == InputEvent::KeyboardEvent::DOWN)
				{
					Log("key down\n");
				}
				else if (event.data.keyboardEvent.type == InputEvent::KeyboardEvent::UP)
				{
					Log("key up\n");
				}
			}break;
			case InputEvent::MOUSE:
			{
				if (event.data.mouseEvent.type == InputEvent::MouseEvent::L_BUTTON_DOWN)
				{
					Log("event time: %f ", ConvertToMs(eventTime - sysInitTime) / 1000.f);
					Log("left button down: %f, %f\n", event.data.mouseEvent.x, event.data.mouseEvent.y);
				}
				if (event.data.mouseEvent.type == InputEvent::MouseEvent::R_BUTTON_DOWN)
				{
					Log("event time: %f ", ConvertToMs(eventTime - sysInitTime) / 1000.f);
					Log("right button down: %f, %f\n", event.data.mouseEvent.x, event.data.mouseEvent.y);
				}
				if (event.data.mouseEvent.type == InputEvent::MouseEvent::WHEEL)
				{
					Log("event time: %f ", ConvertToMs(eventTime - sysInitTime) / 1000.f);
					Log("scroll: %f\n", event.data.mouseEvent.scroll);
				}
			}break;
		}
	}
}

}