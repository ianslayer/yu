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
	bool		captured = false;
	bool		focused = false;

	enum MouseMode
	{
		MOUSE_CAPTURE,
		MOUSE_FREE,
	}; 
	MouseMode	mode = MOUSE_CAPTURE;
};

struct InputEvent
{
	enum Type
	{
		UNKNOWN = 0,
		KEYBOARD,
		MOUSE,
		JOY,

		KILL_FOCUS,
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

	struct JoyEvent
	{
		enum Type
		{
			BUTTON = 1 << 0,
			AXIS = 1 << 1,
		};

		enum BUTTON_ID
		{
			BUTTON_A = 1 << 0,
			BUTTON_B = 1 << 1,
			BUTTON_X = 1 << 2,
			BUTTON_Y = 1 << 3,

			BUTTON_LEFT = 1 << 4,
			BUTTON_RIGHT = 1 << 5,
			BUTTON_UP = 1 << 6,
			BUTTON_DOWN = 1 << 7,

			BUTTON_START = 1 << 8,
			BUTTON_BACK = 1 << 9,

			BUTTON_LEFT_THUMB = 1 << 10,
			BUTTON_RIGHT_THUMB = 1 << 11,

			BUTTON_LEFT_SHOULDER = 1 << 12,
			BUTTON_RIGHT_SHOULDER = 1 << 13,
		};

		u32	type;
		u32	 buttonState;
		int	 controllerIdx;

		float leftX;
		float leftY;

		float rightX;
		float rightY;
	};

	Window		window;
	Type		type;
	union
	{
		MouseEvent		mouseEvent;
		KeyboardEvent	keyboardEvent;
		JoyEvent		joyEvent;
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
	
struct SystemInfo
{
	YU_CLASS_FUNCTION int			NumDisplays();
	YU_CLASS_FUNCTION Display		GetDisplay(int index);
	YU_CLASS_FUNCTION Display		GetMainDisplay();
	YU_CLASS_FUNCTION int			NumDisplayMode(const Display& display);
	//YU_CLASS_FUNCTION Rect			GetDisplayRect(const Display& display);
	YU_CLASS_FUNCTION DisplayMode	GetDisplayMode(const Display& display, int modeIndex);
	YU_CLASS_FUNCTION DisplayMode	GetCurrentDisplayMode(const Display& display);
	//YU_CLASS_FUNCTION void			SetDisplayMode(const Display& display, int modeIndex);
	YU_CLASS_FUNCTION CPUInfo		GetCPUInfo();
};

struct WindowManager
{
	WindowManager() : mgrImpl(0) {}

	Window				CreateWin(const Rect& rect);
	void				CloseWin(Window& win);
	
	bool				DequeueInputEvent(InputEvent& ev);

	Window				mainWindow;
	struct WindowManagerImpl*	mgrImpl;
};

bool	InitWindowManager();
void	FreeWindowManager();

extern WindowManager* gWindowManager;

}

#endif