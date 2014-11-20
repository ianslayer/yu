#include "system.h"

#include <stdio.h>

namespace yu
{
System* gSystem = 0;

bool PlatformInitSystem();

bool InitSystem()
{
	gSystem = new System();

	if(!PlatformInitSystem())
	{
		return false;
	}
	/*
	Display mainDisplay = System::GetMainDisplay();
	
	printf("main display modes:\n");
	
	int numDisplayMode = System::NumDisplayMode(mainDisplay);
	
	for(int i = 0; i < numDisplayMode; i++)
	{
		DisplayMode mode = System::GetDisplayMode(mainDisplay, i);
		printf("width: %lu, height: %lu, refresh rate: %lf\n", mode.width, mode.height, mode.refreshRate);
	}
	
	DisplayMode currentDisplayMode = System::GetCurrentDisplayMode(mainDisplay);
	printf("current mode width: %lu, height: %lu, refresh rate: %lf\n", currentDisplayMode.width, currentDisplayMode.height, currentDisplayMode.refreshRate);
	
	int numDisplay = System::NumDisplays();
	
	printf("display num: %d\n", numDisplay);
	*/

	CPUInfo cpuInfo = System::GetCPUInfo();

	printf("CPU info: \n");
	printf("Vender: %s\n", cpuInfo.vender);

	return true;
}

void FreeSystem()
{


	delete gSystem;
	gSystem = 0;
}

}