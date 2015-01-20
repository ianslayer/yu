#ifndef yu_mac_yu_app_h
#define yu_mac_yu_app_h

#import <Cocoa/Cocoa.h>

namespace yu
{
class WindowManager;
}

@interface YuApp : NSApplication
{
	yu::WindowManager* winManager;
}

@property (assign) IBOutlet NSWindow* mainWin;
-(void) setWindowManager:(yu::WindowManager*) mgr;

-(void) run;
@end

@interface YuView : NSView<NSWindowDelegate>
{
	yu::WindowManager* winManager;
	NSOpenGLContext *openGLContext;
	NSOpenGLPixelFormat *pixelFormat;
}
-(void) setWindowManager:(yu::WindowManager*) mgr;
-(void) initOpenGL;
@end

#endif
