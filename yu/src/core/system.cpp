#include "system.h"

#if defined YU_OS_WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif

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
#endif

void System::GetSysDisplayInfo()
{
#if defined YU_OS_WIN32
	EnumDisplayMonitors(NULL, NULL, MyMonitorEnumProc, 0);
#endif
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