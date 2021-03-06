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

YuApp* gYuApp;

int main(int argc, const char * argv[])
{
	pthread_t yuThread;
	gYuApp = (YuApp*)[YuApp sharedApplication];
	pthread_create(&yuThread, nullptr, yu::YuMainThread, nullptr);
	
	while(! (yu::YuState() != yu::YU_RUNNING)) ;
	
	[gYuApp run];

	return 0;
}
