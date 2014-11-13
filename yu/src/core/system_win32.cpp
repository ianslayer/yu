#include "system.h"
#include <stdio.h>
#include <intrin.h>

namespace yu
{

#define GETX(l) (int(l & 0xFFFF))
#define GETY(l) (int(l) >> 16)
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//    int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_MOUSEMOVE:
		static int lastX, lastY;
		int x, y;
		x = GETX(lParam);
		y = GETY(lParam);
		//if(gInputListener)
		//	gInputListener->OnMouseMove(x, y, x - lastX, y - lastY);
		lastX = x;
		lastY = y;
		break;
	case WM_KEYDOWN:
		//if(gInputListener)
		//	gInputListener->OnKey((unsigned int) wParam, true);
		break;
	case WM_KEYUP:
		//if(gInputListener)
		//	gInputListener->OnKey((unsigned int) wParam, false);
		break;
	case WM_LBUTTONDOWN:
		//if(gInputListener)
		//	gInputListener->OnMouseButton(GETX(lParam), GETY(lParam), MOUSE_LEFT, true);
		break;
	case WM_LBUTTONUP:
		//if(gInputListener)
		//	gInputListener->OnMouseButton(GETX(lParam), GETY(lParam), MOUSE_LEFT, false);
		break;
	case WM_RBUTTONDOWN:
		//if(gInputListener)
		//gInputListener->OnMouseButton(GETX(lParam), GETY(lParam), MOUSE_RIGHT, true);
		break;
	case WM_RBUTTONUP:
		//if(gInputListener)
		//gInputListener->OnMouseButton(GETX(lParam), GETY(lParam), MOUSE_RIGHT, false);
		break;
	case WM_MOUSEWHEEL:
		static int scroll;
		int s;

		scroll += GET_WHEEL_DELTA_WPARAM(wParam);
		s = scroll / WHEEL_DELTA;
		scroll %= WHEEL_DELTA;

		POINT point;
		point.x = GETX(lParam);
		point.y = GETY(lParam);
		ScreenToClient(hWnd, &point);

		//if (s != 0 && gInputListener) gInputListener->OnMouseWheel(point.x, point.y, s);
		break;   

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_SIZE:
		//modify backbuffer size
		UINT width ;
		UINT height;
		RECT clientRect;
		width = GETX(lParam);
		height = GETY(lParam);
		
		GetClientRect(hWnd, &clientRect);
		//ResizeBackBuffer(width, height);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

bool PlatformInitSystem()
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);

	TCHAR* windowClass = TEXT("yuWindow");
	wcex.style			= CS_HREDRAW | CS_VREDRAW ;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= NULL;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= windowClass;
	wcex.hIconSm		= NULL;

	RegisterClassEx(&wcex);

	return true;
}

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
	
	EnumDisplayMonitors(NULL, NULL, GetMainDisplayEnumProc, (LPARAM) &mainDisplay);

	return mainDisplay;
}

DisplayMode System::GetDisplayMode(const Display& display, int modeIndex) const
{
	DisplayMode mode;
	memset(&mode, 0, sizeof(mode));

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

	DWORD windowStyle = WS_CAPTION | WS_SYSMENU | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	TCHAR* windowClass = TEXT("yuWindow");
	TCHAR* windowTitle = TEXT("yu");
	window.hwnd = CreateWindowEx(0, windowClass, windowTitle,
		windowStyle, (int)rect.x, (int)rect.y, (int)rect.width, (int)rect.height, NULL, NULL, NULL, NULL);

	RECT rc = { (LONG)rect.x, (LONG)rect.y, (LONG)rect.x + (LONG)rect.width , (LONG)rect.y + (LONG)rect.height };

	AdjustWindowRect( &rc, windowStyle, FALSE);
	MoveWindow(window.hwnd, (int)rect.x, (int)rect.y, (int)(rc.right - rc.left), (int)(rc.bottom - rc.top), TRUE);

	ShowWindow(window.hwnd, SW_SHOW);

	windowList.PushBack(window);

	return window;
}

void System::CloseWin(Window& win)
{
	for(int i = 0; i < windowList.Size(); i++)
	{
		if(windowList[i].hwnd == win.hwnd)
		{
			windowList.EraseSwapBack(&windowList[i]);
			break;
		}
	}
	CloseWindow(win.hwnd);
}

CPUInfo System::GetCPUInfo() const
{
	CPUInfo cpuInfo;
	memset(&cpuInfo, 0, sizeof(cpuInfo));
	struct Registers
	{
		u32 eax, ebx, ecx, edx;
	};
	struct Str
	{
		char str[4];
	};
	union Info
	{
		Registers	reg;
		Str			str[4];
		int			info[4];
	};

	Info info;

	__cpuid(info.info, 0);
	memcpy(cpuInfo.vender, &info.reg.ebx, sizeof(int));
	memcpy(cpuInfo.vender+4, &info.reg.edx, sizeof(int));
	memcpy(cpuInfo.vender+8, &info.reg.ecx, sizeof(int));

	__cpuid(info.info, 1);
	cpuInfo.featureBits0 = info.reg.ecx;
	cpuInfo.featureBits1 = info.reg.edx;

	return cpuInfo;
}

}