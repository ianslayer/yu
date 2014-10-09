#import <Foundation/Foundation.h>
#import "yu_app.h"

#include "yu.h"

@implementation YuApp

- (void) run
{


	 NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	 _running = YES;
	 yu::InitYu();	
    //YuView* view = [[YuView alloc] initWithFrame:winrect];

   // [win setContentView:view];
   // [win setAcceptsMouseMovedEvents:YES];
   // [win setDelegate:view];
    //[view setApp:pApp];
    //Win = win;
    //View = view;
	
	//[self mainWin] = win;
	//[((NSWindow*)win) makeKeyAndOrderFront:nil];
	
	[self finishLaunching];
	[pool drain];
	
    while ([self isRunning])
    {
        pool = [[NSAutoreleasePool alloc] init];
        NSEvent* event = [self nextEventMatchingMask:NSAnyEventMask untilDate:nil inMode:NSDefaultRunLoopMode dequeue:YES];
        if (event)
        {
            [self sendEvent:event];
        }
		
        [pool drain];
    }
	
	
}

@end

@implementation YuView
-(BOOL) acceptsFirstResponder
{
    return YES;
}
-(BOOL) acceptsFirstMouse:(NSEvent *)ev
{
    return YES;
}

-(BOOL)windowShouldClose:(id)sender
{
	yu::FreeYu();
	exit(0);
    return 1;
}

@end