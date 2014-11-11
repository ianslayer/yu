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
		memcpy(display->device.DeviceName, monInfo.szDevice, sizeof(display->device.DeviceName));
		printf("main monitor name: %s\n", monInfo.szDevice);
	}

	return getMonResult;
}

Display System::GetMainDisplay() const
{
	Display mainDisplay;
	memset(&mainDisplay, 0, sizeof(mainDisplay));
	//EnumDisplayMonitors(NULL, NULL, GetMainDisplayEnumProc, (LPARAM) &mainDisplay);
	
	int index = 0;
	Display display;
	memset(&display, 0, sizeof(display));
	display.device.cb = sizeof(display.device);

	bool mainDisplayFound = false;
	
	while(EnumDisplayDevices(NULL, index, &display.device, 0))
	{
		printf(("Device Name: %s Device String: %s\n"), display.device.DeviceName, display.device.DeviceString);

		Display monitor;
		memset(&monitor, 0, sizeof(monitor));
		monitor.device.cb = sizeof(monitor.device);		
		if(EnumDisplayDevices(display.device.DeviceName, 0, &monitor.device, 0))
		{
			printf(("	Monitor Name: %s Monitor String: %s\n"), monitor.device.DeviceName, monitor.device.DeviceString);

		}
		
		if(display.device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
		{
			mainDisplay = display;
			mainDisplayFound = true;
			break;
		}

		index++;
	}

	if(!mainDisplayFound)
		printf("error: no main display found\n");
	
	return mainDisplay;
}

DisplayMode System::GetDisplayMode(const Display& display, int modeIndex) const
{
	DisplayMode mode;

	DEVMODE devMode;

	memset(&devMode, 0, sizeof(devMode));
	devMode.dmSize = sizeof(devMode);

	int iterModeIndex = 0;
	int supportIndex = 0;

	bool modeFound = false;

	while(EnumDisplaySettingsEx(display.device.DeviceName, iterModeIndex, &devMode, 0))
	{
		if(devMode.dmBitsPerPel > 16 && devMode.dmPelsWidth >= 800)
		{
			if(supportIndex == modeIndex)
			{
				mode.width = devMode.dmPelsWidth;
				mode.height = devMode.dmPelsHeight;
				mode.refreshRate = devMode.dmDisplayFrequency;

				modeFound = true;

				/*
				printf("display mode: %d\n", supportIndex++);
				printf("width: %d height %d\n", devMode.dmPelsWidth, devMode.dmPelsHeight);
				printf("bits per pixel: %d\n", devMode.dmBitsPerPel);
				printf("refresh rate: %d\n", devMode.dmDisplayFrequency);
				*/

				break;
			}

			supportIndex++;
		}

		iterModeIndex++;
	}

	if(!modeFound)
		printf("error: display mode out of range\n");

	//printf("\n");
	
	return mode;
}

DisplayMode System::GetCurrentDisplayMode(const Display& display) const
{
	DisplayMode mode;

	DEVMODE devMode;

	memset(&devMode, 0, sizeof(devMode));
	devMode.dmSize = sizeof(devMode);

	int iterModeIndex = 0;
	int supportIndex = 0;

	bool modeFound = false;

	if(EnumDisplaySettingsEx(display.device.DeviceName, ENUM_CURRENT_SETTINGS, &devMode, 0))
	{
		if(devMode.dmBitsPerPel > 16 && devMode.dmPelsWidth >= 800)
		{
	
				mode.width = devMode.dmPelsWidth;
				mode.height = devMode.dmPelsHeight;
				mode.refreshRate = devMode.dmDisplayFrequency;

				modeFound = true;

				/*
				printf("display mode: %d\n", supportIndex++);
				printf("width: %d height %d\n", devMode.dmPelsWidth, devMode.dmPelsHeight);
				printf("bits per pixel: %d\n", devMode.dmBitsPerPel);
				printf("refresh rate: %d\n", devMode.dmDisplayFrequency);
				*/
		}

		iterModeIndex++;
	}

	if(!modeFound)
		printf("error: display mode not supported\n");

	//printf("\n");

	return mode;
}

int System::NumDisplayMode(const Display& display) const
{

	DEVMODE devMode;

	memset(&devMode, 0, sizeof(devMode));
	devMode.dmSize = sizeof(devMode);

	int iterModeIndex = 0;
	int supportedIndex = 0;

	while(EnumDisplaySettingsEx(display.device.DeviceName, iterModeIndex, &devMode, 0))
	{
		if(devMode.dmBitsPerPel > 16 && devMode.dmPelsWidth >= 800)
		{	
			supportedIndex++;
		}

		iterModeIndex++;
	}

	return supportedIndex;
}

int System::NumDisplays() const
{
	int index = 0;
	int numDisplay = 0;
	Display display;
	memset(&display, 0, sizeof(display));
	display.device.cb = sizeof(display.device);


	while(EnumDisplayDevices(NULL, index, &display.device, 0))
	{
		if(display.device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
		{
			numDisplay++;
			printf(("Device Name: %s Device String: %s\n"), display.device.DeviceName, display.device.DeviceString);

			Display monitor;
			memset(&monitor, 0, sizeof(monitor));
			monitor.device.cb = sizeof(monitor.device);		
			if(EnumDisplayDevices(display.device.DeviceName, 0, &monitor.device, 0))
			{
				printf(("	Monitor Name: %s Monitor String: %s\n"), monitor.device.DeviceName, monitor.device.DeviceString);

			}
		}
		index++;
	}

	return numDisplay;
}

Display System::GetDisplay(int index) const
{
	Display display;
	memset(&display, 0, sizeof(display));

	int displayIndex = 0;
	int activeDisplayIndex = 0;
	bool displayFound = false;
	while(EnumDisplayDevices(NULL, displayIndex, &display.device, 0))
	{
		if(display.device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
		{

			printf(("Device Name: %s Device String: %s\n"), display.device.DeviceName, display.device.DeviceString);

			Display monitor;
			memset(&monitor, 0, sizeof(monitor));
			monitor.device.cb = sizeof(monitor.device);		
			if(EnumDisplayDevices(display.device.DeviceName, 0, &monitor.device, 0))
			{
				printf(("	Monitor Name: %s Monitor String: %s\n"), monitor.device.DeviceName, monitor.device.DeviceString);

			}
			if(activeDisplayIndex == index)
			{
				displayFound	= true;
				break;
			}

			activeDisplayIndex++;
		}
		displayIndex++;
	}

	if(!displayFound)
	{
		printf("error: display index out of bound\n");
	}

	return display;
}


Window System::CreateWin(const Rect& rect)
{
	Window window;
	memset(&window, 0, sizeof(window));

	return window;
}

void System::CloseWin(Window& win)
{
	
}

}