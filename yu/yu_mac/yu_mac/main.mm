//
//  main.m
//  yu_mac
//
//  Created by Yushuo Liou on 9/6/14.
//  Copyright (c) 2014 ianslayer. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include <pthread.h>
#include "../../src/yu.h"
#include "../../src/yu_app.h"
namespace yu
{

void* YuMainThread(void* context)
{
	YuMain();
	return nullptr;
}

}

int main(int argc, const char * argv[])
{
	pthread_t yuThread;
	pthread_create(&yuThread, nullptr, yu::YuMainThread, nullptr);
	NSApplication* yuApp = [YuApp sharedApplication];

	[yuApp run];

	return 0;
}
