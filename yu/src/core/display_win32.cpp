#include "system.h"

#include <stdio.h>

namespace yu
{

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

BOOL CALLBACK GetMainDisplayEnumProc(
	_In_  HMONITOR hMonitor,
	_In_  HDC hdcMonitor,
	_In_  LPRECT lprcMonitor,
	_In_  LPARAM dwData
)
{
	Display* display = (Display*) dwData; 
	MONITORINFOEX monInfo;
	monInfo.cbSize = sizeof(MONITORINFOEX);

	BOOL getMonResult = GetMonitorInfo(hMonitor, &monInfo);

	if(getMonResult && ( monInfo.dwFlags & MONITORINFOF_PRIMARY) )
	{
		display->hMonitor = hMonitor;
	}

	return getMonResult;
}

void System::GetSysDisplayInfo()
{
	EnumDisplayMonitors(NULL, NULL, MyMonitorEnumProc, 0);

	Display display;
	memset(&display, 0, sizeof(display));
	display.device.cb = sizeof(display.device);
	int index = 0;
	while(EnumDisplayDevices(NULL, index, &display.device, 0))
	{
		displayList.PushBack(display);
		index++;
	}
}

Display System::GetMainDisplay() const
{
	Display mainDisplay;
	memset(&mainDisplay, 0, sizeof(mainDisplay));
	EnumDisplayMonitors(NULL, NULL, GetMainDisplayEnumProc, (LPARAM) &mainDisplay);
	
	int index = 0;
	Display display;
	memset(&display, 0, sizeof(display));
	display.device.cb = sizeof(display.device);
	while(EnumDisplayDevices(NULL, index, &display.device, 0))
	{
		printf(("Device Name: %s Device String: %s\n"), display.device.DeviceName, display.device.DeviceString);

		/*
		if(EnumDisplayDevices(display.device.DeviceName, 0, &display.device, 0))
		{
			printf(("Monitor Name: %s Monitor String: %s\n"), display.device.DeviceName, display.device.DeviceString);
		}
		*/
		if(display.device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
		{
			mainDisplay = display;
		}

		index++;
	}
	
	return mainDisplay;
}

DisplayMode System::GetDisplayMode(const Display& display, int modeIndex) const
{
	DisplayMode mode;

	return mode;
}

}