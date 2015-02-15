#include "../container/dequeue.h"
#include "../container/array.h"

#include "timer.h"
#include "system_impl.h"
#include "thread_impl.h"
#include "worker_impl.h"
#include "log_impl.h"
#include "allocator_impl.h"
#include "string_impl.h"

#include "../renderer/renderer.h"
#include <Mmsystem.h>
#pragma comment(lib, "Winmm.lib")

#include <xinput.h>

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(XInputGetStateFuncPtr);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}

XInputGetStateFuncPtr* dllXInputGetState = XInputGetStateStub;

namespace yu
{
void SetYuExit();
bool YuRunning();
void ResizeBackBuffer(unsigned int width, unsigned int height, TextureFormat fmt);

YU_INTERNAL void InitXInput()
{
	HMODULE xinputDll = LoadLibraryA("xinput1_4.dll");
	if (!xinputDll)
	{
		xinputDll = LoadLibraryA("xinput9_1_0.dll");
	}
	if (!xinputDll)
	{
		xinputDll = LoadLibraryA("xinput1_3.dll");
	}

	if (xinputDll)
	{
		dllXInputGetState = (XInputGetStateFuncPtr*)GetProcAddress(xinputDll, "XInputGetState");
	}
}

YU_INTERNAL void CaptureMouse(Window& win)
{
	if (win.captured)
		return; 

	int			width, height;
	RECT		windowRect;

	width = GetSystemMetrics(SM_CXSCREEN);
	height = GetSystemMetrics(SM_CYSCREEN);

	GetWindowRect(win.hwnd, &windowRect);
	if (windowRect.left < 0)
		windowRect.left = 0;
	if (windowRect.top < 0)
		windowRect.top = 0;
	if (windowRect.right >= width)
		windowRect.right = width - 1;
	if (windowRect.bottom >= height - 1)
		windowRect.bottom = height - 1;
	int windowCenterX = (windowRect.right + windowRect.left) / 2;
	int windowCenterY = (windowRect.top + windowRect.bottom) / 2;

	BOOL setPosSuccess = SetCursorPos(windowCenterX, windowCenterY);

	SetCapture(win.hwnd);
	ClipCursor(&windowRect);
	while (ShowCursor(FALSE) >= 0)
		;

	win.captured = true;

}

YU_INTERNAL void DecaptureMouse()
{
	ClipCursor(NULL);
	ReleaseCapture();
	while (ShowCursor(TRUE) < 0)
		;
}

YU_INTERNAL void EnqueueEvent(InputEvent& ev)
{
	ev.timeStamp = SampleTime().time;
	gWindowManager->mgrImpl->inputQueue.Enqueue(ev);
}

YU_INTERNAL void GetMousePos(const Window& win)
{
	POINT	currentPos;
	GetCursorPos(&currentPos);

	int			width, height;
	RECT		windowRect;
	width = GetSystemMetrics(SM_CXSCREEN);
	height = GetSystemMetrics(SM_CYSCREEN);

	GetWindowRect(win.hwnd, &windowRect);

	if (windowRect.left < 0)
		windowRect.left = 0;
	if (windowRect.top < 0)
		windowRect.top = 0;
	if (windowRect.right >= width)
		windowRect.right = width - 1;
	if (windowRect.bottom >= height - 1)
		windowRect.bottom = height - 1;
	int windowCenterX = (windowRect.right + windowRect.left) / 2;
	int windowCenterY = (windowRect.top + windowRect.bottom) / 2;

	BOOL setPosSuccess = SetCursorPos(windowCenterX, windowCenterY);

	int dx = currentPos.x - windowCenterX;
	int dy = currentPos.y - windowCenterY;

	InputEvent ev;
	size_t evSize = sizeof(ev);
	ev.type = InputEvent::MOUSE;
	ev.window = win;
	ev.mouseEvent.type = InputEvent::MouseEvent::MOVE;
	ev.mouseEvent.x = float(dx);
	ev.mouseEvent.y = float(dy);
	
	EnqueueEvent(ev);
}

#define GETX(l) (int(l & 0xFFFF))
#define GETY(l) (int(l) >> 16)
YU_INTERNAL LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	InputEvent ev = {};

	RECT clientRect;
	GetClientRect(hWnd, &clientRect);

	Window* win = nullptr;
	for (int i = 0; i < gWindowManager->mgrImpl->windowList.Size(); i++)
	{
		if (gWindowManager->mgrImpl->windowList[i].hwnd == hWnd)
			win = &gWindowManager->mgrImpl->windowList[i];
	}

	if (!win)
	{
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	ev.window = *win;
	short x, y;
	switch (message)
	{
	case WM_KEYDOWN:
		if (lParam & (1<<30))
		{
			break;
		}
		ev.type = InputEvent::KEYBOARD;
		ev.keyboardEvent.type = InputEvent::KeyboardEvent::DOWN;
		ev.keyboardEvent.key = (unsigned int)wParam;
		
		break;
	case WM_KEYUP:
		ev.type = InputEvent::KEYBOARD;
		ev.keyboardEvent.type = InputEvent::KeyboardEvent::UP;
		ev.keyboardEvent.key = (unsigned int)wParam;
		break;

	//case WM_MOVE:

	//	break;
	case WM_MOUSEMOVE:
		x = GETX(lParam);
		y = GETY(lParam);

		if (x < 0 || x >(clientRect.right - clientRect.left)) break;
		if (y < 0 || y >(clientRect.bottom - clientRect.top)) break;

		if (win->mode == Window::MOUSE_CAPTURE) break;
		//if (!win->focused) break;
		ev.type = InputEvent::MOUSE;

		ev.mouseEvent.type = InputEvent::MouseEvent::MOVE;
		ev.mouseEvent.x = float(x);
		ev.mouseEvent.y = float(y);
		

		break;
	case WM_LBUTTONDOWN:
		x = GETX(lParam);
		y = GETY(lParam);

		if (x < 0 || x >(clientRect.right - clientRect.left)) break;
		if (y < 0 || y >(clientRect.bottom - clientRect.top)) break;

		ev.type = InputEvent::MOUSE;

		ev.mouseEvent.type = InputEvent::MouseEvent::L_BUTTON_DOWN;
		ev.mouseEvent.x = float(x);
		ev.mouseEvent.y = float(y);

		break;
	case WM_LBUTTONUP:
		x = GETX(lParam);
		y = GETY(lParam);

		if (x < 0 || x >(clientRect.right - clientRect.left)) break;
		if (y < 0 || y >(clientRect.bottom - clientRect.top)) break;

		ev.type = InputEvent::MOUSE;

		ev.mouseEvent.type = InputEvent::MouseEvent::L_BUTTON_UP;
		ev.mouseEvent.x = float(x);
		ev.mouseEvent.y = float(y);

		break;
	case WM_RBUTTONDOWN:
		x = GETX(lParam);
		y = GETY(lParam);

		if (x < 0 || x >(clientRect.right - clientRect.left)) break;
		if (y < 0 || y >(clientRect.bottom - clientRect.top)) break;

		ev.type = InputEvent::MOUSE;

		ev.mouseEvent.type = InputEvent::MouseEvent::R_BUTTON_DOWN;
		ev.mouseEvent.x = float(x);
		ev.mouseEvent.y = float(y);

		break;
	case WM_RBUTTONUP:
		x = GETX(lParam);
		y = GETY(lParam);

		if (x < 0 || x >(clientRect.right - clientRect.left)) break;
		if (y < 0 || y >(clientRect.bottom - clientRect.top)) break;

		ev.type = InputEvent::MOUSE;

		ev.mouseEvent.type = InputEvent::MouseEvent::R_BUTTON_UP;
		ev.mouseEvent.x = float(x);
		ev.mouseEvent.y = float(y);

		break;
	case WM_MOUSEWHEEL:
		ev.type = InputEvent::MOUSE;
		int scroll;
		scroll = GET_WHEEL_DELTA_WPARAM(wParam);

		ev.mouseEvent.type = InputEvent::MouseEvent::WHEEL;
		ev.mouseEvent.scroll = float(scroll) / float(WHEEL_DELTA);
		
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
		ResizeBackBuffer(width, height, TextureFormat::TEX_FORMAT_R8G8B8A8_UNORM);
		break;
	case WM_MOUSEACTIVATE:
	case WM_ACTIVATE:
	{
		int	active, minimized;

		active = LOWORD(wParam);
		minimized = (BOOL)HIWORD(wParam);

		win->focused = (active == WA_ACTIVE || active == WA_CLICKACTIVE);
		//CaptureMouse(*win);
	}
	break;
	case WM_KILLFOCUS:
	{
		win->focused = false;
		win->captured = false;
		ev.type = InputEvent::KILL_FOCUS;

		DecaptureMouse();
	}
	break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	if (ev.type != InputEvent::UNKNOWN)
		EnqueueEvent(ev);
	return 0;
}

YU_INTERNAL void ExecWindowCommand(WindowThreadCmd& cmd)
{
	switch (cmd.type)
	{
		case (WindowThreadCmd::CREATE_WINDOW):
		{
			CreateWinParam* param = cmd.createWinParam;
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

			if (window.mode == Window::MOUSE_CAPTURE)
			{
				SetFocus(window.hwnd); //TODO: FIXME, this will not send WM_ACTIVATE...
				window.focused = true; //so this is the hack to make mouse work properly
				CaptureMouse(window);
			}

			gWindowManager->mgrImpl->windowList.PushBack(window);
			param->win = window;
			param->winCreationCS.Unlock();

			NotifyCondVar(param->winCreationCV);
		}
		break;
		
		case(WindowThreadCmd::CLOSE_WINDOW) :
		{
			CloseWinParam* param = cmd.closeWinParam;
			param->winCloseCS.Lock();
			for (int i = 0; i < gWindowManager->mgrImpl->windowList.Size(); i++)
			{
				if (gWindowManager->mgrImpl->windowList[i].hwnd == param->win.hwnd)
				{
					gWindowManager->mgrImpl->windowList.EraseSwapBack(&gWindowManager->mgrImpl->windowList[i]);
					break;
				}
			}
			CloseWindow(param->win.hwnd);
			param->winCloseCS.Unlock();
			NotifyCondVar(param->winCloseCV);
		}
		break;

	}
}
YU_INTERNAL u32 TranslateButton(WORD xInputButton, DWORD xInputButtonId, InputEvent::JoyEvent::BUTTON_ID buttonId)
{
	u32 button;
	button = (xInputButton & xInputButtonId) ? buttonId : 0;
	return button;
}

#define XINPUT_MAX_AXIS 32767
#define XINPUT_MIN_AXIS (-32768)
YU_INTERNAL float TranslateAxis(SHORT rawValue, float deadZone)
{
	float value = 0.f;

	if (rawValue> deadZone || rawValue < -deadZone)
	{
		value = clip(rawValue, (SHORT)XINPUT_MIN_AXIS, (SHORT)XINPUT_MAX_AXIS);
		if (value >= 0)
			value = (value - deadZone) / ((float)XINPUT_MAX_AXIS - deadZone);
		else
			value = -(-value - deadZone) / ((float)-XINPUT_MIN_AXIS - deadZone);
	}
	return value;
}

YU_INTERNAL void ProcessXInput(int controllerIdx, const XINPUT_STATE& state, const XINPUT_STATE& oldState)
{
	InputEvent ev = {};
	ev.type = InputEvent::JOY;

	ev.joyEvent.type |= InputEvent::JoyEvent::AXIS;
	ev.joyEvent.leftX = state.Gamepad.sThumbLX;
	ev.joyEvent.controllerIdx = controllerIdx;

	ev.joyEvent.leftX = TranslateAxis(state.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
	ev.joyEvent.leftY = TranslateAxis(state.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
	ev.joyEvent.rightX = TranslateAxis(state.Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
	ev.joyEvent.rightY = TranslateAxis(state.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);

	if (state.Gamepad.wButtons != oldState.Gamepad.wButtons)
	{
		ev.joyEvent.type |= InputEvent::JoyEvent::BUTTON;
		WORD xInputButton = state.Gamepad.wButtons;
		u32 button = 0;
		button |= TranslateButton(xInputButton, XINPUT_GAMEPAD_DPAD_UP, InputEvent::JoyEvent::BUTTON_UP);
		button |= TranslateButton(xInputButton, XINPUT_GAMEPAD_DPAD_DOWN, InputEvent::JoyEvent::BUTTON_DOWN);
		button |= TranslateButton(xInputButton, XINPUT_GAMEPAD_DPAD_LEFT, InputEvent::JoyEvent::BUTTON_LEFT);
		button |= TranslateButton(xInputButton, XINPUT_GAMEPAD_DPAD_RIGHT, InputEvent::JoyEvent::BUTTON_RIGHT);

		button |= TranslateButton(xInputButton, XINPUT_GAMEPAD_A, InputEvent::JoyEvent::BUTTON_A);
		button |= TranslateButton(xInputButton, XINPUT_GAMEPAD_B, InputEvent::JoyEvent::BUTTON_B);
		button |= TranslateButton(xInputButton, XINPUT_GAMEPAD_X, InputEvent::JoyEvent::BUTTON_X);
		button |= TranslateButton(xInputButton, XINPUT_GAMEPAD_Y, InputEvent::JoyEvent::BUTTON_Y);

		button |= TranslateButton(xInputButton, XINPUT_GAMEPAD_LEFT_THUMB, InputEvent::JoyEvent::BUTTON_LEFT_THUMB);
		button |= TranslateButton(xInputButton, XINPUT_GAMEPAD_RIGHT_THUMB, InputEvent::JoyEvent::BUTTON_RIGHT_THUMB);

		button |= TranslateButton(xInputButton, XINPUT_GAMEPAD_LEFT_SHOULDER, InputEvent::JoyEvent::BUTTON_LEFT_SHOULDER);
		button |= TranslateButton(xInputButton, XINPUT_GAMEPAD_RIGHT_SHOULDER, InputEvent::JoyEvent::BUTTON_RIGHT_SHOULDER);

		ev.joyEvent.buttonState = button;
	}

	EnqueueEvent(ev);
}

YU_INTERNAL ThreadReturn ThreadCall WindowThreadFunc(ThreadContext context)
{
	WindowManagerImpl* mgrImpl = (WindowManagerImpl*)context;

	MSG msg = { 0 };

	timeBeginPeriod(1);
	DWORD xInputPacket = 0;
	XINPUT_STATE oldState = {};
	while (YuRunning())
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			//while (GetMessage(&msg, NULL, 0, 0))
		{

			WindowThreadCmd cmd;
			while (mgrImpl->winThreadCmdQueue.Dequeue(cmd))
			{
				ExecWindowCommand(cmd);
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);

		}

		for (int i = 0; i < mgrImpl->windowList.Size(); i++)
		{
			if (mgrImpl->windowList[i].mode == Window::MOUSE_CAPTURE && mgrImpl->windowList[i].focused)
			{
				CaptureMouse(mgrImpl->windowList[i]);
				GetMousePos(mgrImpl->windowList[i]);
			}
		}

		DWORD result;
		for (int i = 0; i < XUSER_MAX_COUNT; i++)
		{
			XINPUT_STATE state;
			yu::memset(&state, 0, sizeof(state));
			result = dllXInputGetState(i, &state);

			if (result == ERROR_SUCCESS)
			{
				if (state.dwPacketNumber != xInputPacket)
				{
					xInputPacket = state.dwPacketNumber;
					//Log(" xinput input packet: %d\n", xInputPacket);
					ProcessXInput(i, state, oldState);
					oldState = state;
				}
			}
			else
			{

			}

		}


		Sleep(1);
	}
	
	//OutputDebugStringA("exit window thread\n");
	return 0;
}

Window WindowManager::CreateWin(const Rect& rect)
{
	WindowManagerImpl* mgrImpl = this->mgrImpl;
	CreateWinParam param;
	WindowThreadCmd cmd;
	param.rect = rect;

	param.winCreationCS.Lock();
	cmd.type = WindowThreadCmd::CREATE_WINDOW;
	cmd.createWinParam = &param;

	while (1)
	{
		BOOL success = PostThreadMessage(mgrImpl->windowThread.handle, WM_APP, 0, 0);//wake window thread
		if (mgrImpl->winThreadCmdQueue.Enqueue(cmd))
		{
			success = PostThreadMessage(mgrImpl->windowThread.handle, WM_APP, 0, 0);
			WaitForCondVar(param.winCreationCV, param.winCreationCS);
			break;
		}
	}
	param.winCreationCS.Unlock();

	return param.win;
}

void WindowManager::CloseWin(Window& win)
{
	WindowManagerImpl* mgrImpl = this->mgrImpl;
	CloseWinParam param;
	WindowThreadCmd cmd;
	param.win = win;

	param.winCloseCS.Lock();
	cmd.type = WindowThreadCmd::CLOSE_WINDOW;
	cmd.closeWinParam = &param;
	while (1)
	{
		BOOL success = PostThreadMessage(mgrImpl->windowThread.handle, WM_APP, 0, 0);//wake window thread
		if (mgrImpl->winThreadCmdQueue.Enqueue(cmd))
		{
			success = PostThreadMessage(mgrImpl->windowThread.handle, WM_APP, 0, 0);
			WaitForCondVar(param.winCloseCV, param.winCloseCS);
			break;
		}
	}
	param.winCloseCS.Unlock();
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

	WindowManagerImpl* mgrImpl = New<WindowManagerImpl>(gSysArena);
	gWindowManager->mgrImpl = mgrImpl;
	mgrImpl->windowThread = CreateThread(WindowThreadFunc, mgrImpl);
	SetThreadName(mgrImpl->windowThread.handle, "Window Thread");

	InitXInput();

	return true;
}

YU_INTERNAL BOOL CALLBACK MyMonitorEnumProc(
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

YU_INTERNAL BOOL CALLBACK GetMainDisplayEnumProc(
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

Display SystemInfo::GetMainDisplay()
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

DisplayMode SystemInfo::GetDisplayMode(const Display& display, int modeIndex)
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

DisplayMode SystemInfo::GetCurrentDisplayMode(const Display& display)
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

int SystemInfo::NumDisplayMode(const Display& display)
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

int SystemInfo::NumDisplays()
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

Display SystemInfo::GetDisplay(int index)
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

struct ThreadRunnerContext
{
	ThreadPriority	priority;
	u64				affinityMask;
	ThreadFunc*		func;
	ThreadContext	context;

	CondVar			threadCreationCV;
	Mutex			threadCreationCS;
};
void RegisterThread(ThreadHandle thread);
void ThreadExit(ThreadHandle handle);

ThreadHandle GetCurrentThreadHandle()
{
	DWORD threadId = GetThreadId(GetCurrentThread());

	return threadId;

	/*
	DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &threadHandle,
	DUPLICATE_SAME_ACCESS, FALSE, DUPLICATE_SAME_ACCESS);
	*/
}

ThreadReturn ThreadCall ThreadRunner(ThreadContext context)
{
	ThreadRunnerContext* runnerContext = (ThreadRunnerContext*)context;
	runnerContext->threadCreationCS.Lock();

	ThreadHandle threadHandle = GetCurrentThreadHandle();
	if (runnerContext->affinityMask)
		SetThreadAffinity(threadHandle, runnerContext->affinityMask);

	RegisterThread(threadHandle);

	//subtle, but this two must be copied before notify,, else runnerContext my be destroied before they can be executed
	ThreadFunc* func = runnerContext->func;
	ThreadContext threadContext = runnerContext->context;

	runnerContext->threadCreationCS.Unlock();
	NotifyCondVar(runnerContext->threadCreationCV);

	ThreadReturn ret;
	ret = func(threadContext);

	ThreadExit(threadHandle);

	return ret;
}

Thread CreateThread(ThreadFunc func, ThreadContext context, ThreadPriority priority, u64 affinityMask)
{
	Thread thread;

	ThreadRunnerContext runnerContext;
	runnerContext.threadCreationCS.Lock();
	runnerContext.priority = priority;
	runnerContext.affinityMask = affinityMask;
	runnerContext.func = func;
	runnerContext.context = context;

	DWORD threadId;
	::CreateThread(NULL, 0, ThreadRunner, &runnerContext, 0, &threadId);
	thread.handle = threadId;

	WaitForCondVar(runnerContext.threadCreationCV, runnerContext.threadCreationCS);
	runnerContext.threadCreationCS.Unlock();

	return thread;
}

u64 GetThreadAffinity(ThreadHandle threadHandle)
{
	GROUP_AFFINITY groupAffinity = {};
	HANDLE realHandle = OpenThread(THREAD_ALL_ACCESS, FALSE, threadHandle);
	if (realHandle)
	{
		::GetThreadGroupAffinity(realHandle, &groupAffinity);
		CloseHandle(realHandle);
		return groupAffinity.Mask;
	}
	return 0;
}

void SetThreadAffinity(ThreadHandle threadHandle, u64 affinityMask)
{
	if (affinityMask)
	{
		HANDLE realHandle = OpenThread(THREAD_ALL_ACCESS, FALSE, threadHandle);
		if (realHandle)
		{
			::SetThreadAffinityMask(realHandle, affinityMask);
			CloseHandle(realHandle);
		}
	}
}

const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // Must be 0x1000.
	LPCSTR szName; // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1=caller thread).
	DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void SetThreadName(ThreadHandle threadHandle, const char* name)
{
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = name;
	info.dwThreadID = threadHandle;
	info.dwFlags = 0;

	__try
	{
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
}

Mutex::Mutex()
{
	InitializeCriticalSectionAndSpinCount(&m, 1000);
}

Mutex::~Mutex()
{
	DeleteCriticalSection(&m);
}

void Mutex::Lock()
{
	EnterCriticalSection(&m);
}

void Mutex::Unlock()
{
	LeaveCriticalSection(&m);
}

CondVar::CondVar()
{
	InitializeConditionVariable(&cv);
}

CondVar::~CondVar()
{
	WakeConditionVariable(&cv);
}

void WaitForCondVar(CondVar& cv, Mutex& m)
{
	SleepConditionVariableCS(&cv.cv, &m.m, INFINITE);
}

void NotifyCondVar(CondVar& cv)
{
	WakeConditionVariable(&cv.cv);
}

void NotifyAllCondVar(CondVar& cv)
{
	WakeAllConditionVariable(&cv.cv);
}

Semaphore::Semaphore(int initCount, int maxCount)
{
	sem = CreateSemaphore(nullptr, initCount, maxCount, nullptr);
}

Semaphore::~Semaphore()
{
	CloseHandle(sem);
}

void WaitForSem(Semaphore& sem)
{
	WaitForSingleObject(sem.sem, INFINITE);
}

void SignalSem(Semaphore& sem)
{
	LONG prevCount;
	BOOL result = ReleaseSemaphore(sem.sem, 1, &prevCount);
	if (!result)
	{
		Log("error, sem signal failed");
	}

}

size_t FileSize(const char* path)
{
	HANDLE fileHandle = CreateFileA(path, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	LARGE_INTEGER fileSize;
	BOOL getSizeSuccess = GetFileSizeEx(fileHandle, &fileSize);
	CloseHandle(fileHandle);
	if (getSizeSuccess)
	{
		return fileSize.QuadPart;
	}
	return 0;
}


size_t ReadFile(const char* path, void* buffer, size_t bufferLen)
{

	size_t fileSize = FileSize(path);

	HANDLE fileHandle = CreateFileA(path, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	DWORD readSize;
	BOOL readSuccess = ::ReadFile(fileHandle, buffer, (DWORD)min(fileSize, bufferLen), &readSize, nullptr);
	CloseHandle(fileHandle);

	return readSize;
}


YU_GLOBAL CycleCount initCycle;
YU_GLOBAL u64        cpuFrequency;

YU_GLOBAL Time initTime;
YU_GLOBAL Time frameStartTime;
YU_GLOBAL u64 timerFrequency;

const CycleCount& PerfTimer::Duration() const
{
	return cycleCounter;
}

f64 PerfTimer::DurationInMs() const
{
	return ConvertToMs(cycleCounter);
}

Time SampleTime()
{
	Time time;
	LARGE_INTEGER perfCount;
	QueryPerformanceCounter(&perfCount);
	time.time = perfCount.QuadPart;

	return time;
}

Time SysStartTime()
{
	return initTime;
}

void Timer::Start()
{
	time = SampleTime();
}

void Timer::Stop()
{
	Time finishTime = SampleTime();
	time.time = finishTime.time - time.time;
}

const Time& Timer::Duration() const
{
	return time;
}

f64 Timer::DurationInMs() const
{
	return ConvertToMs(time);
}

void InitSysTime()
{
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	timerFrequency = frequency.QuadPart;

	initCycle = SampleCycle();
	cpuFrequency = EstimateCPUFrequency();

	initTime = SampleTime();

	Log("estimated cpu freq: %llu\n", cpuFrequency);
	Log("timer freq: %llu\n", timerFrequency);
}

u64 EstimateCPUFrequency()
{
	ThreadHandle currentThreadHandle = GetCurrentThreadHandle();
	u64 originAffinity = GetThreadAffinity(currentThreadHandle);
	SetThreadAffinity(currentThreadHandle, 1);

	Time startCount = SampleTime();
	Time duration = {};

	PerfTimer timer;
	timer.Start();
	while (ConvertToMs(duration) < 1000)
	{
		Time curTime = SampleTime();
		duration = curTime - startCount;
	}
	timer.Stop();
	SetThreadAffinity(currentThreadHandle, originAffinity);

	return timer.Duration().cycle;
}

f64 ConvertToMs(const CycleCount& cycles)
{
	i64 time = (i64)(cycles.cycle / cpuFrequency);  // unsigned->sign conversion should be safe here
	i64 timeFract = (i64)(cycles.cycle % cpuFrequency);  // unsigned->sign conversion should be safe here
	f64 ret = (time)+(f64)timeFract / (f64)((i64)cpuFrequency);
	return ret * 1000.0;
}

f64 ConvertToMs(const Time& time)
{
	u64 timeInMs = time.time * 1000;
	return (f64)timeInMs / (f64)timerFrequency;
}

void DummyWorkLoad(double timeInMs)
{
	Time startTime = SampleTime();
	double deltaT = 0;
	while (1)
	{
		if (deltaT >= timeInMs)
			break;
		Time endTime = SampleTime();

		deltaT = ConvertToMs(endTime - startTime);
	}
}

}