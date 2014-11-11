#include "system.h"

#include <stdio.h>

namespace yu
{
System* gSystem = 0;
	
void InitSystem()
{
	gSystem = new System();
	//gSystem->GetSysDisplayInfo();
	Display mainDisplay = gSystem->GetMainDisplay();
	
	printf("main display modes:\n");
	
	int numDisplayMode = gSystem->NumDisplayMode(mainDisplay);
	
	for(int i = 0; i < numDisplayMode; i++)
	{
		DisplayMode mode = gSystem->GetDisplayMode(mainDisplay, i);
		printf("width: %lu, height: %lu, refresh rate: %lf\n", mode.width, mode.height, mode.refreshRate);
	}
	
	DisplayMode currentDisplayMode = gSystem->GetCurrentDisplayMode(mainDisplay);
	printf("current mode width: %lu, height: %lu, refresh rate: %lf\n", currentDisplayMode.width, currentDisplayMode.height, currentDisplayMode.refreshRate);
	
	int numDisplay = gSystem->NumDisplays();
	
	printf("display num: %d\n", numDisplay);

}

void FreeSystem()
{
	for(int i = 0 ; i < gSystem->windowList.Size(); i++)
	{
		gSystem->CloseWin(gSystem->windowList[i]);
	}

	delete gSystem;
	gSystem = 0;
}

}