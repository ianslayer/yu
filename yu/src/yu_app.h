#ifndef yu_mac_yu_app_h
#define yu_mac_yu_app_h

#import <Cocoa/Cocoa.h>

namespace yu
{
class System;
}

@interface YuApp : NSApplication

@property (assign) IBOutlet NSWindow* mainWin;
@property yu::System* system;

-(void) run;
@end

@interface YuView : NSView<NSWindowDelegate>

@end

#endif
