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

struct InputEvent
{
	enum Type
	{
		UNKNOWN = 0,
		KEYBOARD,
		MOUSE,
	};

	struct KeyboardEvent
	{
		enum Type
		{
			DOWN,
			UP,
		};
		Type	type;
		unsigned int key;
	};

	struct MouseEvent
	{
		enum Type
		{
			MOVE,
			L_BUTTON_DOWN,
			L_BUTTON_UP,
			R_BUTTON_DOWN,
			R_BUTTON_UP,
			WHEEL,
		};
		Type	type;
		float	x;
		float	y;
		float	scroll;
	};

	Window		window;
	Type		type;
	union
	{
		MouseEvent		mouseEvent;
		KeyboardEvent	keyboardEvent;
	};
	u64			timeStamp;
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
	char	vender[16];
#if defined (YU_CPU_X86_64) || defined (YU_CPU_X86)

	bool	x2apic = false;
	bool	sse = false;
	bool	sse2 = false;
	bool	sse3 = false;
	bool	ssse3 = false;
	bool	sse4_1 = false;
	bool	sse4_2 = false;
	bool	avx = false;
	bool	avx2 = false;
	
	bool	tsc = false;
	bool	invariantTsc = false;

#endif
	u32		numLogicalProcessors = 0;
	u32		numLogicalProcessorsPerCore = 0;
};
	
struct GPUInfo
{

};

class System
{
public:
	System() : sysImpl(0) {}
	virtual ~System();
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

	void*				GetInputQueue(); //hack, ... I don't want include dequeue.h

	friend bool			InitSystem();

	Window				mainWindow;
	class SystemImpl*	sysImpl;

};

bool	InitSystem();
void	FreeSystem();

extern System* gSystem;

}

#endif