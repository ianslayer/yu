#ifndef yu_mac_yu_app_h
#define yu_mac_yu_app_h

#import <Cocoa/Cocoa.h>

namespace yu
{
class WindowManager;
}

@interface YuApp : NSApplication
{
}
@property (assign) yu::WindowManager* winManager;
@property (assign) IBOutlet NSWindow* mainWin;

-(void) run;
@end

@interface YuWindow : NSWindow
@end

@interface YuView : NSView<NSWindowDelegate>
{
}
@property (assign) yu::WindowManager* winManager;
@property (assign) NSOpenGLPixelFormat* pixelFormat;
@property (assign) NSOpenGLContext* openGLContext;
-(void) initOpenGL;
@end

#endif
