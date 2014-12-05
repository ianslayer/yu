#include "system_impl.h"
#include "../renderer/renderer.h"

namespace yu
{
void SetYuExit();
bool YuRunning();
void ResizeBackBuffer(unsigned int width, unsigned int height, TexFormat fmt);
#define GETX(l) (int(l & 0xFFFF))
#define GETY(l) (int(l) >> 16)
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	InputEvent ev = {};
	ev.timeStamp = SampleTime().time;
	int x, y;
	switch (message)
	{
	case WM_KEYDOWN:
		if (lParam & (1<<30))
		{
			break;
		}
		ev.type = InputEvent::KEYBOARD;
		ev.data.keyboardEvent.type = InputEvent::KeyboardEvent::DOWN;
		ev.data.keyboardEvent.key = (unsigned int)wParam;
		
		break;
	case WM_KEYUP:
		ev.type = InputEvent::KEYBOARD;
		ev.data.keyboardEvent.type = InputEvent::KeyboardEvent::UP;
		ev.data.keyboardEvent.key = (unsigned int)wParam;
		break;
	case WM_MOUSEMOVE:
		ev.type = InputEvent::MOUSE;

		x = GETX(lParam);
		y = GETY(lParam);

		ev.data.mouseEvent.type = InputEvent::MouseEvent::MOVE;
		ev.data.mouseEvent.x = float(x);
		ev.data.mouseEvent.y = float(y);
		
		break;
	case WM_LBUTTONDOWN:
		ev.type = InputEvent::MOUSE;

		x = GETX(lParam);
		y = GETY(lParam);

		ev.data.mouseEvent.type = InputEvent::MouseEvent::L_BUTTON_DOWN;
		ev.data.mouseEvent.x = float(x);
		ev.data.mouseEvent.y = float(y);

		break;
	case WM_LBUTTONUP:
		ev.type = InputEvent::MOUSE;

		x = GETX(lParam);
		y = GETY(lParam);

		ev.data.mouseEvent.type = InputEvent::MouseEvent::L_BUTTON_UP;
		ev.data.mouseEvent.x = float(x);
		ev.data.mouseEvent.y = float(y);

		break;
	case WM_RBUTTONDOWN:
		ev.type = InputEvent::MOUSE;

		x = GETX(lParam);
		y = GETY(lParam);

		ev.data.mouseEvent.type = InputEvent::MouseEvent::R_BUTTON_DOWN;
		ev.data.mouseEvent.x = float(x);
		ev.data.mouseEvent.y = float(y);

		break;
	case WM_RBUTTONUP:
		ev.type = InputEvent::MOUSE;

		x = GETX(lParam);
		y = GETY(lParam);

		ev.data.mouseEvent.type = InputEvent::MouseEvent::R_BUTTON_UP;
		ev.data.mouseEvent.x = float(x);
		ev.data.mouseEvent.y = float(y);

		break;
	case WM_MOUSEWHEEL:
		ev.type = InputEvent::MOUSE;
		int scroll;
		scroll = GET_WHEEL_DELTA_WPARAM(wParam);

		ev.data.mouseEvent.type = InputEvent::MouseEvent::WHEEL;
		ev.data.mouseEvent.scroll = float(scroll) / float(WHEEL_DELTA);
		
		break;   

	case WM_DESTROY:
		SetYuExit();
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

		RECT windowRect;
		GetWindowRect(hWnd, &windowRect);
		ResizeBackBuffer(width, height, TexFormat::TEX_FORMAT_R8G8B8A8_UNORM);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	if (ev.type != InputEvent::UNKNOWN)
		gSystem->sysImpl->inputQueue.Enqueue(ev);
	return 0;
}

void ExecWindowCommand(WindowThreadCmd& cmd)
{
	switch (cmd.type)
	{
		case (WindowThreadCmd::CREATE_WINDOW):
		{
			CreateWinParam* param = (CreateWinParam*)cmd.cmd.createWinParam;
			param->winCreationCS.Lock();
			Rect rect = param->rect;
			Window window = {};

			DWORD windowStyle = WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
			TCHAR* windowClass = TEXT("yuWindow");
			TCHAR* windowTitle = TEXT("yu");
			window.hwnd = CreateWindowEx(0, windowClass, windowTitle,
				windowStyle, (int)rect.x, (int)rect.y, (int)rect.width, (int)rect.height, NULL, NULL, NULL, NULL);

			RECT rc = { (LONG)rect.x, (LONG)rect.y, (LONG)rect.x + (LONG)rect.width, (LONG)rect.y + (LONG)rect.height };

			AdjustWindowRect(&rc, windowStyle, FALSE);
			MoveWindow(window.hwnd, (int)rect.x, (int)rect.y, (int)(rc.right - rc.left), (int)(rc.bottom - rc.top), TRUE);

			ShowWindow(window.hwnd, SW_SHOW);

			((SystemImpl*)(gSystem->sysImpl))->windowList.PushBack(window);
			param->win = window;
			param->winCreationCS.Unlock();

			NotifyCondVar(param->winCreationCV);
		}
		break;


	}
}


ThreadReturn ThreadCall WindowThreadFunc(ThreadContext context)
{
	SystemImpl* sysImpl = (SystemImpl*)context;

	MSG msg = { 0 };

	while ( YuRunning())
	{
		//while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		while (GetMessage(&msg, NULL, 0, 0))
		{
			WindowThreadCmd cmd;
			while (sysImpl->winThreadCmdQueue.Dequeue(cmd))
			{
				ExecWindowCommand(cmd);
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	
	for (int i = 0; i < ((SystemImpl*)(gSystem->sysImpl))->windowList.Size(); i++)
	{
		gSystem->CloseWin(((SystemImpl*)(gSystem->sysImpl))->windowList[i]);
	}

	OutputDebugStringA("exit window thread\n");
	return 0;
}

bool PlatformInitSystem()
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);

	TCHAR* windowClass = TEXT("yuWindow");
	wcex.style			= CS_OWNDC | CS_HREDRAW | CS_VREDRAW ;
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

	SystemImpl* sysImpl = new SystemImpl();
	gSystem->sysImpl = sysImpl;
	sysImpl->windowThread = CreateThread(WindowThreadFunc, sysImpl);
	SetThreadName(sysImpl->windowThread.handle, "Window Thread");
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
		Log("main monitor name: %s\n", monInfo.szDevice);
	}

	return getMonResult;
}

Display System::GetMainDisplay()
{
	Display mainDisplay={};
	
	int index = 0;
	Display display={};
	display.device.cb = sizeof(display.device);

	bool mainDisplayFound = false;
	
	while(EnumDisplayDevices(NULL, index, &display.device, 0))
	{
		Log(("Device Name: %s Device String: %s\n"), display.device.DeviceName, display.device.DeviceString);

		Display monitor={};
		monitor.device.cb = sizeof(monitor.device);		
		if(EnumDisplayDevices(display.device.DeviceName, 0, &monitor.device, 0))
		{
			Log(("	Monitor Name: %s Monitor String: %s\n"), monitor.device.DeviceName, monitor.device.DeviceString);

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
		Log("error: no main display found\n");
	
	EnumDisplayMonitors(NULL, NULL, GetMainDisplayEnumProc, (LPARAM) &mainDisplay);

	return mainDisplay;
}

DisplayMode System::GetDisplayMode(const Display& display, int modeIndex)
{
	DisplayMode mode={};

	DEVMODE devMode = {};

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
				Log("display mode: %d\n", supportIndex++);
				Log("width: %d height %d\n", devMode.dmPelsWidth, devMode.dmPelsHeight);
				Log("bits per pixel: %d\n", devMode.dmBitsPerPel);
				Log("refresh rate: %d\n", devMode.dmDisplayFrequency);
				*/

				break;
			}

			supportIndex++;
		}

		iterModeIndex++;
	}

	if(!modeFound)
		Log("error: display mode out of range\n");

	//Log("\n");
	
	return mode;
}

DisplayMode System::GetCurrentDisplayMode(const Display& display)
{
	DisplayMode mode = {};

	DEVMODE devMode={};
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
				Log("display mode: %d\n", supportIndex++);
				Log("width: %d height %d\n", devMode.dmPelsWidth, devMode.dmPelsHeight);
				Log("bits per pixel: %d\n", devMode.dmBitsPerPel);
				Log("refresh rate: %d\n", devMode.dmDisplayFrequency);
				*/
		}

		iterModeIndex++;
	}

	if(!modeFound)
		Log("error: display mode not supported\n");

	//Log("\n");

	return mode;
}

int System::NumDisplayMode(const Display& display)
{

	DEVMODE devMode={};
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

int System::NumDisplays()
{
	int index = 0;
	int numDisplay = 0;
	Display display={};
	display.device.cb = sizeof(display.device);

	while(EnumDisplayDevices(NULL, index, &display.device, 0))
	{
		if(display.device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
		{
			numDisplay++;
			Log(("Device Name: %s Device String: %s\n"), display.device.DeviceName, display.device.DeviceString);

			Display monitor={};
			monitor.device.cb = sizeof(monitor.device);
			if(EnumDisplayDevices(display.device.DeviceName, 0, &monitor.device, 0))
			{
				Log(("	Monitor Name: %s Monitor String: %s\n"), monitor.device.DeviceName, monitor.device.DeviceString);

			}
		}
		index++;
	}

	return numDisplay;
}

Display System::GetDisplay(int index)
{
	Display display={};

	int displayIndex = 0;
	int activeDisplayIndex = 0;
	bool displayFound = false;
	while(EnumDisplayDevices(NULL, displayIndex, &display.device, 0))
	{
		if(display.device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
		{

			Log(("Device Name: %s Device String: %s\n"), display.device.DeviceName, display.device.DeviceString);

			Display monitor={};
			monitor.device.cb = sizeof(monitor.device);		
			if(EnumDisplayDevices(display.device.DeviceName, 0, &monitor.device, 0))
			{
				Log(("	Monitor Name: %s Monitor String: %s\n"), monitor.device.DeviceName, monitor.device.DeviceString);

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
		Log("error: display index out of bound\n");
	}

	return display;
}

Window System::CreateWin(const Rect& rect)
{
	SystemImpl* sys = (SystemImpl*)(this->sysImpl);
	CreateWinParam param;
	WindowThreadCmd cmd;
	param.rect = rect;

	param.winCreationCS.Lock();
	cmd.type = WindowThreadCmd::CREATE_WINDOW;
	cmd.cmd.createWinParam = &param;


	SystemImpl* sysImpl = (SystemImpl*)gSystem->sysImpl;

	//sysImpl->windowThread = CreateThread(WindowThreadFunc, sysImpl);
	//SetThreadName(sysImpl->windowThread.threadHandle, "Window Thread");

	while (1)
	{
		BOOL success = PostThreadMessage(sysImpl->windowThread.handle, WM_APP, 0, 0);//wake window thread
		if (sys->winThreadCmdQueue.Enqueue(cmd))
		{
			success = PostThreadMessage(sysImpl->windowThread.handle, WM_APP, 0, 0);
			WaitForCondVar(param.winCreationCV, param.winCreationCS);
			break;
		}
	}
	param.winCreationCS.Unlock();

	return param.win;
}

void System::CloseWin(Window& win)
{
	for(int i = 0; i < ((SystemImpl*)sysImpl)->windowList.Size(); i++)
	{
		if (((SystemImpl*)sysImpl)->windowList[i].hwnd == win.hwnd)
		{
			((SystemImpl*)sysImpl)->windowList.EraseSwapBack(&((SystemImpl*)sysImpl)->windowList[i]);
			break;
		}
	}
	CloseWindow(win.hwnd);
}

}