#include "system.h"
#include "thread.h"
#include <ApplicationSErvices/ApplicationServices.h>
#import <Cocoa/Cocoa.h>

namespace yu
{

bool PlatformInitSystem()
{
	return true;
}

#define MAX_DISPLAY 16

int System::NumDisplays()
{
	CGDirectDisplayID displayIds[MAX_DISPLAY];
	uint32_t numDisplay;
	CGGetOnlineDisplayList(MAX_DISPLAY, displayIds, &numDisplay);
	
	return (int) numDisplay;
}

/*
Rect System::GetDisplayRect(const Display& display)
{
	Rect rect;
	CGRect cgRect = CGDisplayBounds(display.id);
	
	rect.x = cgRect.origin.x;
	rect.y = cgRect.origin.y;
	rect.width = cgRect.size.width;
	rect.height = cgRect.size.height;
	
	return rect;
}
*/

CGDirectDisplayID DisplayFromScreen(NSScreen *s)
{
    NSNumber* didref = (NSNumber*)[[s deviceDescription] objectForKey:@"NSScreenNumber"];
    CGDirectDisplayID disp = (CGDirectDisplayID)[didref longValue];
    return disp;
}

NSScreen* ScreenFromDisplay(const Display& display)
{
	NSArray* screens = [NSScreen screens];
	
	for (id object in screens) {
		NSScreen* screen = (NSScreen*) object;
		CGDirectDisplayID id = DisplayFromScreen(screen);
		
		if(id == display.id)
		{
			return screen;
		}
	}
	
	return nullptr;
}

Display System::GetDisplay(int index)
{
	CGDirectDisplayID displayIds[MAX_DISPLAY];
	uint32_t numDisplay;
	CGGetOnlineDisplayList(MAX_DISPLAY, displayIds, &numDisplay);
	
	Display display = {};
	
	if(index < numDisplay)
	{
		display.id = displayIds[index];
	}

	display.screen = ScreenFromDisplay(display);

	return display;
}

Display System::GetMainDisplay()
{
	Display display = {};
	//memset(&display, 0, sizeof(display));
	display.id = CGMainDisplayID();
	display.screen = ScreenFromDisplay(display);
	return display;
}

int System::NumDisplayMode(const yu::Display &display)
{
	CFArrayRef displayModeList = CGDisplayCopyAllDisplayModes(display.id, NULL);
	CFIndex totalModes = CFArrayGetCount(displayModeList);
	CFRelease(displayModeList);
	
	return (int) totalModes;
}

DisplayMode System::GetDisplayMode(const yu::Display &display, int modeIndex)
{
	DisplayMode displayMode = {};
	
	CFArrayRef displayModeList = CGDisplayCopyAllDisplayModes(display.id, NULL);
	
	if(modeIndex < CFArrayGetCount(displayModeList))
	{
		CGDisplayModeRef mode = (CGDisplayModeRef) CFArrayGetValueAtIndex(displayModeList, modeIndex);
		
		displayMode.refreshRate = CGDisplayModeGetRefreshRate(mode);
		displayMode.width = CGDisplayModeGetPixelWidth(mode);
		displayMode.height = CGDisplayModeGetPixelHeight(mode);

	}

	CFRelease(displayModeList);
	
	return displayMode;
}

DisplayMode System::GetCurrentDisplayMode(const Display& display)
{
	DisplayMode displayMode;
	CGDisplayModeRef mode = CGDisplayCopyDisplayMode(display.id);
	displayMode.refreshRate = CGDisplayModeGetRefreshRate(mode);
	displayMode.width = CGDisplayModeGetPixelWidth(mode);
	displayMode.height = CGDisplayModeGetPixelHeight(mode);
	CFRelease(mode);
	return displayMode;
}

/*
void System::SetDisplayMode(const Display& display, int modeIndex)
{
	if(modeIndex < NumDisplayMode(display))
	{
		
	}
}
*/

Window	System::CreateWin(const Rect& rect)
{
	Window window = {};
	
    NSRect winrect;
    winrect.origin.x = rect.x;
    winrect.origin.y = rect.y;
    winrect.size.width = rect.width;
    winrect.size.height = rect.height;
	
    NSWindow* win = [[NSWindow alloc] initWithContentRect:winrect styleMask:NSTitledWindowMask|NSClosableWindowMask backing:NSBackingStoreBuffered defer:NO];
	
	[win setAcceptsMouseMovedEvents:YES];
	[((NSWindow*)win) makeKeyAndOrderFront:nil];
	
	window.win = win;
	
	//windowList.PushBack(window);
	
	return window;
}

void System::CloseWin(yu::Window &win)
{
	//for(int i = 0; i < windowList.Size(); i++)
	{
	//	if(win.win == windowList[i].win)
		{
			[win.win release];
	//		windowList.Erase(i);
	//		break;
		}
	}
}

/*
void System::GetSysDisplayInfo()
{
	CGDirectDisplayID displayIds[16];
	uint32_t numDisplay;
	CGDirectDisplayID mainDisplayId;
	
	mainDisplayId = CGMainDisplayID();
	CGGetOnlineDisplayList(16, displayIds, &numDisplay);
	
	for(unsigned int i = 0; i < numDisplay; i++)
	{
		Display display;
		display.id = displayIds[i];
		if(displayIds[i] == mainDisplayId)
		{
			mainDisplayIndex = i;
		}
		
		displayList.PushBack(display);
	}
}*/


}