#ifndef yu_mac_yu_app_h
#define yu_mac_yu_app_h

#import <Cocoa/Cocoa.h>

namespace yu
{
class System;
}

@interface YuApp : NSApplication
{
	yu::System* system;
}

@property (assign) IBOutlet NSWindow* mainWin;
-(void) setSystem:(yu::System*) sys;

-(void) run;
@end

@interface YuView : NSView<NSWindowDelegate>
{
	yu::System* system;
	NSOpenGLContext *openGLContext;
	NSOpenGLPixelFormat *pixelFormat;
}
-(void) setSystem:(yu::System*) sys;
-(void) initOpenGL;
@end

#endif
