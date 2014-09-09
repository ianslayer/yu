#ifndef YU_SYSTEM_H
#define YU_SYSTEM_H

#include "platform.h"
#ifdef YU_OS_WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
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

	bool	fullScreen;
};

struct Display
{

};

class System
{
public:

	void			GetSysDisplayInfo();

	Array<Window>	windowList;
	Array<Display>	displayList;
};

void	InitSystem();
void	FreeSystem();

extern System* gSystem;

}

#endif