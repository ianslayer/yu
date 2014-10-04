#ifndef YU_SYSTEM_H
#define YU_SYSTEM_H

#include "platform.h"
#if defined YU_OS_WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
#elif defined YU_OS_MAC
#endif

#include "../container/array.h"

namespace yu
{

struct Event
{

};

struct Window
{
#ifdef YU_OS_WIN32
    HWND	hwnd;
#endif

	//client area;
	int		width;
	int		height;

};

struct DisplayMode
{
	size_t		width;
	size_t		height;
	double		x,y;
	double		refreshRate;
};

struct Display
{

#if defined YU_OS_MAC
	u32 id; //CGDirectDisplayID
#endif
	
	int numDisplayMode;
	
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
	int				NumDisplayMode() const;
	DisplayMode		GetDisplayMode(const Display& display, int modeIndex) const;

	friend void		InitSystem();
private:

	void			GetSysDisplayInfo();
	Array<Window>	windowList;
	Array<Display>	displayList;
	int				mainDisplayIndex;
	
	
};

void	InitSystem();
void	FreeSystem();

extern System* gSystem;

}

#endif