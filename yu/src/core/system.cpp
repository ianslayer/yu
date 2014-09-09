#include "system.h"

#ifdef YU_OS_WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif

namespace yu
{
System* gSystem = 0;

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

void InitSystem()
{
	gSystem = new System();
	gSystem->GetSysDisplayInfo();
}

void FreeSystem()
{
	delete gSystem;
	gSystem = 0;
}

}