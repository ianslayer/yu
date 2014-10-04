#include "system.h"

#if defined YU_OS_WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif

#include <stdio.h>

namespace yu
{
System* gSystem = 0;

#if defined YU_OS_WIN32
BOOL CALLBACK MyMonitorEnumProc(
	_In_  HMONITOR hMonitor,
	_In_  HDC hdcMonitor,
	_In_  LPRECT lprcMonitor,
	_In_  LPARAM dwData
)
{
	MONITORINFOEX monInfo;
	monInfo.cbSize = sizeof(MONITORINFOEX);

	BOOL getMonResult = GetMonitorInfo(hMonitor, &monInfo);

	return getMonResult;
}

void System::GetSysDisplayInfo()
{
	EnumDisplayMonitors(NULL, NULL, MyMonitorEnumProc, 0);
}
#endif
	
void InitSystem()
{
	gSystem = new System();
	gSystem->GetSysDisplayInfo();
	Display mainDisplay = gSystem->GetMainDisplay();
	
	printf("main display modes:");
	for(int i = 0; i < mainDisplay.numDisplayMode; i++)
	{
		DisplayMode mode = gSystem->GetDisplayMode(mainDisplay, i);
		printf("width: %lu, height: %lu, refresh rate: %lf\n", mode.width, mode.height, mode.refreshRate);
	}
	
}

void FreeSystem()
{
	delete gSystem;
	gSystem = 0;
}

}