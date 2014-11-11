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

#include "../container/array.h"

namespace yu
{

struct Event
{

};

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
};
	
struct GPUInfo
{
};
	
class System
{
public:

	int				NumDisplays() const;
	Display			GetDisplay(int index) const;
	Display			GetMainDisplay() const;
	int				NumDisplayMode(const Display& display) const;
	Rect			GetDisplayRect(const Display& display) const;
	DisplayMode		GetDisplayMode(const Display& display, int modeIndex) const;
	DisplayMode		GetCurrentDisplayMode(const Display& display) const;
	void			SetDisplayMode(const Display& display, int modeIndex);

	Window			CreateFullScreenWindow(const Display& display);
	Window			CreateWin(const Rect& rect);
	void			CloseWin(Window& win);
	

	friend void		InitSystem();
	
	Window			mainWindow;
	Array<Window>	windowList;
private:
	
};

void	InitSystem();
void	FreeSystem();

extern System* gSystem;

}

#endif