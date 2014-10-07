#include "system.h"

#include <stdio.h>

namespace yu
{
System* gSystem = 0;


	
void InitSystem()
{
	gSystem = new System();
	gSystem->GetSysDisplayInfo();
	Display mainDisplay = gSystem->GetMainDisplay();
	
	printf("main display modes:");
	for(int i = 0; i < mainDisplay.numDisplayMode; i++)
	{
		DisplayMode mode = gSystem->GetDisplayMode(mainDisplay, i);
		printf("width: %lu, height: %lu, refresh rate: %lf\n", mode.width, mode.height, mode.refreshRate);
	}
	
}

void FreeSystem()
{
	delete gSystem;
	gSystem = 0;
}

}