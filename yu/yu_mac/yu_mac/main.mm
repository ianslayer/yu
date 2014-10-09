//
//  main.m
//  yu_mac
//
//  Created by Yushuo Liou on 9/6/14.
//  Copyright (c) 2014 ianslayer. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include "../../src/yu_app.h"

int main(int argc, const char * argv[])
{
	NSApplication* yuApp = [YuApp sharedApplication];
	
	[yuApp run];

	return 0;
}
