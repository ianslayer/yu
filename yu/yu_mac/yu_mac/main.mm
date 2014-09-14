//
//  main.m
//  yu_mac
//
//  Created by Yushuo Liou on 9/6/14.
//  Copyright (c) 2014 ianslayer. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include "../../src/container/array.h"
#include "../../src/core/timer.h"

int main(int argc, const char * argv[])
{
	yu::InitDefaultAllocator();
	yu::InitSysTime();
	yu::Array<int> intArray;
	intArray.PushBack(10);
	
	
	yu::PerfTimer perfTimer;
	perfTimer.cycleCounter.cycle = 0;
	perfTimer.Start();
	perfTimer.Finish();
	
	yu::u64 cpuFreq = yu::EstimateCPUFrequency();
	
	printf("empty per timer cycles: %llu\n", perfTimer.cycleCounter.cycle);
	printf("estimate cpu freq: %llu\n", cpuFreq);
	
	return NSApplicationMain(argc, argv);
}
