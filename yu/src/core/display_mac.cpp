#include "system.h"
#include <ApplicationSErvices/ApplicationServices.h>

namespace yu
{

#define MAX_DISPLAY 16

int System::NumDisplays() const
{
	CGDirectDisplayID displayIds[MAX_DISPLAY];
	uint32_t numDisplay;
	CGGetOnlineDisplayList(MAX_DISPLAY, displayIds, &numDisplay);
	
	return (int) numDisplay;
}


Display System::GetDisplay(int index) const
{
	CGDirectDisplayID displayIds[MAX_DISPLAY];
	uint32_t numDisplay;
	CGGetOnlineDisplayList(MAX_DISPLAY, displayIds, &numDisplay);
	
	Display display;
	memset(&display, 0, sizeof(display));
	
	if(index < numDisplay)
	{
		display.id = displayIds[index];
		CFArrayRef displayModeList = CGDisplayCopyAllDisplayModes(display.id, NULL);
		CFIndex totalModes = CFArrayGetCount(displayModeList);
		
		display.numDisplayMode = (int) totalModes;
		
		CFRelease(displayModeList);
	}
	
	return display;
}

Display System::GetMainDisplay() const
{
	Display display;
	memset(&display, 0, sizeof(display));
	display.id = CGMainDisplayID();
	
	CFArrayRef displayModeList = CGDisplayCopyAllDisplayModes(display.id, NULL);
	CFIndex totalModes = CFArrayGetCount(displayModeList);
		
	display.numDisplayMode = (int) totalModes;
		
	CFRelease(displayModeList);
	
	return display;
}

DisplayMode System::GetDisplayMode(const yu::Display &display, int modeIndex) const
{
	DisplayMode displayMode;
	memset(&displayMode, 0, sizeof(displayMode));
	CFArrayRef displayModeList = CGDisplayCopyAllDisplayModes(display.id, NULL);
	
	if(modeIndex < CFArrayGetCount(displayModeList))
	{
		CGDisplayModeRef mode = (CGDisplayModeRef) CFArrayGetValueAtIndex(displayModeList, modeIndex);

		CGRect rect = CGDisplayBounds(display.id);
		displayMode.refreshRate = CGDisplayModeGetRefreshRate(mode);
		displayMode.x = rect.origin.x;
		displayMode.y = rect.origin.y;
		displayMode.width = CGDisplayModeGetPixelWidth(mode);
		displayMode.height = CGDisplayModeGetPixelHeight(mode);
	}
	
	CFRelease(displayModeList);
	return displayMode;
}

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
		
		CGRect rect = CGDisplayBounds(display.id);
		size_t height = CGDisplayPixelsHigh(display.id);
		size_t width = CGDisplayPixelsWide(display.id);
		
		CFArrayRef displayModeList = CGDisplayCopyAllDisplayModes(display.id, NULL);
		
		CFIndex totalModes = CFArrayGetCount(displayModeList);
		for(CFIndex modeIdx = 0; modeIdx < totalModes; modeIdx++)
		{
			CGDisplayModeRef mode = (CGDisplayModeRef) CFArrayGetValueAtIndex(displayModeList, modeIdx);
			double refreshRate = CGDisplayModeGetRefreshRate(mode);
			
			DisplayMode dispMode;
			
			dispMode.refreshRate = refreshRate;
			dispMode.width = CGDisplayModeGetPixelWidth(mode);
			dispMode.height = CGDisplayModeGetPixelHeight(mode);
			
		}
		
		CFRelease(displayModeList);
		
		displayList.PushBack(display);
	}
	
}
	
}