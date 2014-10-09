#ifndef yu_mac_yu_app_h
#define yu_mac_yu_app_h

#import <Cocoa/Cocoa.h>

@interface YuApp : NSApplication

@property (assign) IBOutlet NSWindow* mainWin;

-(void) run;

@end

@interface YuView : NSView<NSWindowDelegate>

@end

#endif
