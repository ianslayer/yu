#ifndef YU_SYSTEM_H
#define YU_SYSTEM_H

#include "platform.h"
#if defined YU_OS_WIN32
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <Windows.h>
#elif defined YU_OS_MAC //ugly hack to put NS object into C struct
	#if defined __OBJC__
		@class NSScreen;
		@class NSWindow;
	#else
		typedef void NSScreen;
		typedef void NSWindow;
	#endif
#endif

namespace yu
{

struct Rect
{
	double		width;
	double		height;
	double		x,y;
};

struct Window
{
#if defined YU_OS_WIN32
    HWND		hwnd;
#elif defined YU_OS_MAC
	NSWindow*	win;
#endif

};

struct DisplayMode
{
	size_t		width;
	size_t		height;
	double		refreshRate;
};

struct Display
{
#if defined YU_OS_WIN32
	HMONITOR		hMonitor;
	DISPLAY_DEVICE	device;
#elif defined YU_OS_MAC
	u32				id; //CGDirectDisplayID
	NSScreen*		screen; //NSScreen
#endif
	
};

struct CPUInfo
{
#if defined (YU_CPU_X86_64) || defined (YU_CPU_X86)
	char	vender[16];
	char	brand[40];
	u32		featureBits0;	//ecx
	u32		featureBits1;	//edx
#endif
};
	
struct GPUInfo
{
};
	
class System
{
public:

	static int			NumDisplays();
	static Display		GetDisplay(int index);
	static Display		GetMainDisplay();
	static int			NumDisplayMode(const Display& display);
	//static Rect			GetDisplayRect(const Display& display);
	static DisplayMode	GetDisplayMode(const Display& display, int modeIndex);
	static DisplayMode	GetCurrentDisplayMode(const Display& display);
	//static void			SetDisplayMode(const Display& display, int modeIndex);

	Window				CreateWin(const Rect& rect);
	void				CloseWin(Window& win);
	
	static CPUInfo		GetCPUInfo();

	friend bool			InitSystem();

	Window				mainWindow;
	System*				sysImpl;

};

bool	InitSystem();
void	FreeSystem();

extern System* gSystem;

}

#endif