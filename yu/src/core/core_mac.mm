#include <ApplicationSErvices/ApplicationServices.h>
#import <Cocoa/Cocoa.h>
#include <mach/mach_time.h>
#import "yu_app.h"

#include "../container/array.h"
#include "../container/dequeue.h"

#include "log_impl.h"
#include "timer_impl.h"
#include "string_impl.h"
#include "system_impl.h"
#include "allocator_impl.h"
#include "thread_posix_impl.h"
#include "worker_impl.h"
#include "file_posix_impl.h"

namespace yu
{

void InitSysTime()
{
	mach_timebase_info_data_t timeInfo;
	mach_timebase_info(&timeInfo);
	timerFrequency = timeInfo.numer / timeInfo.denom;

	initCycle = SampleCycle();
	cpuFrequency = EstimateCPUFrequency();
	
	initTime = SampleTime();

	Log("estimated cpu freq: %llu\n", cpuFrequency);
	Log("timer freq: %llu\n", timerFrequency);
}

Time SampleTime()
{
	Time time;
	time.time = mach_absolute_time();
	return time;
}

f64 ConvertToMs(const Time& time)
{
	mach_timebase_info_data_t timeInfo;
	mach_timebase_info(&timeInfo);
	
	f64 ret = ((time.time * timeInfo.numer) / (timeInfo.denom)) / (1000000.0);
	
	return ret;
}


void ExecWindowCommand(WindowThreadCmd& cmd)
{
	switch (cmd.type)
	{
		case (WindowThreadCmd::CREATE_WINDOW):
		{
			CreateWinParam* param = (CreateWinParam*)cmd.createWinParam;
			param->winCreationCS.Lock();
			Rect rect = param->rect;
			Window window = {};


			NSRect winrect;
			winrect.origin.x = rect.x;
			winrect.origin.y = rect.y;
			winrect.size.width = rect.width;
			winrect.size.height = rect.height;
	
			NSWindow* win = [[NSWindow alloc] initWithContentRect:winrect styleMask:NSTitledWindowMask|NSClosableWindowMask backing:NSBackingStoreBuffered defer:NO];
	
			[win setAcceptsMouseMovedEvents:YES];
			[((NSWindow*)win) makeKeyAndOrderFront:nil];
			[NSApp activateIgnoringOtherApps:YES];
			
			YuView* view = [[YuView alloc] initWithFrame:winrect];
			[view setWindowManager:gWindowManager];
			[win setContentView:view];
			[win setAcceptsMouseMovedEvents:YES];
			[win setDelegate:view];
			
			
			window.win = win;

			(gWindowManager->mgrImpl)->windowList.PushBack(window);
			param->win = window;
			param->winCreationCS.Unlock();

			NotifyCondVar(param->winCreationCV);
		}
		break;


	}
}

}

@implementation YuApp
-(void) setWindowManager:(yu::WindowManager*) mgr
{
	winManager = mgr;
}
- (void) run
{
	 NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	 _running = YES;
	
	[self finishLaunching];
	[pool drain];
	
	//NSRunLoop* loop = [NSRunLoop currentRunLoop];
	//while([loop runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]])

    while ([self isRunning])
    {
        pool = [[NSAutoreleasePool alloc] init];
        NSEvent* event = [self nextEventMatchingMask:NSAnyEventMask untilDate:nil inMode:NSDefaultRunLoopMode dequeue:YES];
        if (event)
        {
            [self sendEvent:event];
        }
		
		yu::WindowManagerImpl* mgrImpl = winManager->mgrImpl;
		yu::WindowThreadCmd cmd;
		while (mgrImpl->winThreadCmdQueue.Dequeue(cmd))
		{
			ExecWindowCommand(cmd);
		}
		
        [pool drain];
    }
}

@end

@implementation YuView
-(void) setWindowManager:(yu::WindowManager*) winMgr
{
	winManager = winMgr;
}

-(NSOpenGLContext*) openGLContext
{
	return openGLContext;
}

- (void) lockFocus
{
	[super lockFocus];
	if ([openGLContext view] != self)
		[openGLContext setView:self];
}

- (id) initWithFrame:(NSRect)frameRect
{
	[super initWithFrame:frameRect];
	return self;
}

-(void) initOpenGL
{
    NSOpenGLPixelFormatAttribute attribs[] =
    {
	/*
		kCGLPFAAccelerated,
		kCGLPFANoRecovery,
		kCGLPFADoubleBuffer,
		kCGLPFAColorSize, 24,
		kCGLPFADepthSize, 16,
		0*/
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
//        NSOpenGLPFAWindow,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFADepthSize, 24,
        0
		
    };
	
    pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
	
    if (!pixelFormat)
		NSLog(@"No OpenGL pixel format");
	
	// NSOpenGLView does not handle context sharing, so we draw to a custom NSView instead
	openGLContext = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
	
		// Synchronize buffer swaps with vertical refresh rate
	GLint swapInt = 0;
	[openGLContext setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];
	
	[openGLContext makeCurrentContext];
}

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
	//yu::FreeYu();
	exit(0);
    return 1;
}

- (void)mouseDown:(NSEvent *)theEvent
{
	NSPoint eventLocation = [theEvent locationInWindow];
	yu::InputEvent ev = {};
	ev.type = yu::InputEvent::MOUSE;

	NSRect winrect = [self bounds];
	
	ev.mouseEvent.type = yu::InputEvent::MouseEvent::L_BUTTON_DOWN;
	ev.mouseEvent.x = float(eventLocation.x);
	ev.mouseEvent.y = float(winrect.size.height - eventLocation.y);
	
	winManager->EnqueueEvent(ev);
}
 
- (void)mouseUp:(NSEvent *)theEvent
{
	NSPoint eventLocation = [theEvent locationInWindow];
	yu::InputEvent ev = {};
	ev.type = yu::InputEvent::MOUSE;

	NSRect winrect = [self bounds];
	
	ev.mouseEvent.type = yu::InputEvent::MouseEvent::L_BUTTON_UP;
	ev.mouseEvent.x = float(eventLocation.x);
	ev.mouseEvent.y = float(winrect.size.height - eventLocation.y);
	
	winManager->EnqueueEvent(ev);
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
	NSPoint eventLocation = [theEvent locationInWindow];
	yu::InputEvent ev = {};
	ev.type = yu::InputEvent::MOUSE;
	NSRect winrect = [self bounds];
	
	ev.mouseEvent.type = yu::InputEvent::MouseEvent::R_BUTTON_DOWN;
	ev.mouseEvent.x = float(eventLocation.x);
	ev.mouseEvent.y = float(winrect.size.height - eventLocation.y);
	
	winManager->EnqueueEvent(ev);
}

- (void)rightMouseUp:(NSEvent *)theEvent
{
	NSPoint eventLocation = [theEvent locationInWindow];
	yu::InputEvent ev = {};
	ev.type = yu::InputEvent::MOUSE;

	NSRect winrect = [self bounds];
	
	ev.mouseEvent.type = yu::InputEvent::MouseEvent::R_BUTTON_UP;
	ev.mouseEvent.x = float(eventLocation.x);
	ev.mouseEvent.y = float(winrect.size.height - eventLocation.y);
	
	winManager->EnqueueEvent(ev);
}

-(void)mouseMoved:(NSEvent *)theEvent
{
	NSPoint eventLocation = [theEvent locationInWindow];
	yu::InputEvent ev = {};
	ev.type = yu::InputEvent::MOUSE;

	NSRect winrect = [self bounds];
	
	ev.mouseEvent.type = yu::InputEvent::MouseEvent::MOVE;
	ev.mouseEvent.x = float(eventLocation.x);
	ev.mouseEvent.y = float(winrect.size.height - eventLocation.y);
	
	winManager->EnqueueEvent(ev);
}

-(void)keyDown:(NSEvent *)theEvent
{
	yu::InputEvent ev = {};
	ev.type = yu::InputEvent::KEYBOARD;
	ev.keyboardEvent.type = yu::InputEvent::KeyboardEvent::DOWN;
	unichar key = [[theEvent charactersIgnoringModifiers] characterAtIndex:0];
	if(key >= 0 && key < 256)
	{
		char asciiKey = yu::ToUpperCase((char) key);
		ev.keyboardEvent.key = asciiKey;
		winManager->EnqueueEvent(ev);
	}
}

-(void)keyUp:(NSEvent *)theEvent
{
	yu::InputEvent ev = {};
	ev.type = yu::InputEvent::KEYBOARD;
	ev.keyboardEvent.type = yu::InputEvent::KeyboardEvent::UP;
	unichar key = [[theEvent charactersIgnoringModifiers] characterAtIndex:0];
	if(key >= 0 && key < 256)
	{
		char asciiKey = yu::ToUpperCase((char) key);
		ev.keyboardEvent.key = asciiKey;
		winManager->EnqueueEvent(ev);
	}
}

@end

extern YuApp* gYuApp;
namespace yu
{
bool PlatformInitSystem()
{
	gWindowManager->mgrImpl = new WindowManagerImpl();
	[gYuApp setWindowManager: gWindowManager];
	return true;
}

#define MAX_DISPLAY 16

int SystemInfo::NumDisplays()
{
	CGDirectDisplayID displayIds[MAX_DISPLAY];
	uint32_t numDisplay;
	CGGetOnlineDisplayList(MAX_DISPLAY, displayIds, &numDisplay);
	
	return (int) numDisplay;
}

/*
Rect System::GetDisplayRect(const Display& display)
{
	Rect rect;
	CGRect cgRect = CGDisplayBounds(display.id);
	
	rect.x = cgRect.origin.x;
	rect.y = cgRect.origin.y;
	rect.width = cgRect.size.width;
	rect.height = cgRect.size.height;
	
	return rect;
}
*/

CGDirectDisplayID DisplayFromScreen(NSScreen *s)
{
    NSNumber* didref = (NSNumber*)[[s deviceDescription] objectForKey:@"NSScreenNumber"];
    CGDirectDisplayID disp = (CGDirectDisplayID)[didref longValue];
    return disp;
}

NSScreen* ScreenFromDisplay(const Display& display)
{
	NSArray* screens = [NSScreen screens];
	
	for (id object in screens) {
		NSScreen* screen = (NSScreen*) object;
		CGDirectDisplayID id = DisplayFromScreen(screen);
		
		if(id == display.id)
		{
			return screen;
		}
	}
	
	return nullptr;
}

Display SystemInfo::GetDisplay(int index)
{
	CGDirectDisplayID displayIds[MAX_DISPLAY];
	uint32_t numDisplay;
	CGGetOnlineDisplayList(MAX_DISPLAY, displayIds, &numDisplay);
	
	Display display = {};
	
	if(index < numDisplay)
	{
		display.id = displayIds[index];
	}

	display.screen = ScreenFromDisplay(display);

	return display;
}

Display SystemInfo::GetMainDisplay()
{
	Display display = {};
	//memset(&display, 0, sizeof(display));
	display.id = CGMainDisplayID();
	display.screen = ScreenFromDisplay(display);
	return display;
}

int SystemInfo::NumDisplayMode(const yu::Display &display)
{
	CFArrayRef displayModeList = CGDisplayCopyAllDisplayModes(display.id, NULL);
	CFIndex totalModes = CFArrayGetCount(displayModeList);
	CFRelease(displayModeList);
	
	return (int) totalModes;
}

DisplayMode SystemInfo::GetDisplayMode(const yu::Display &display, int modeIndex)
{
	DisplayMode displayMode = {};
	
	CFArrayRef displayModeList = CGDisplayCopyAllDisplayModes(display.id, NULL);
	
	if(modeIndex < CFArrayGetCount(displayModeList))
	{
		CGDisplayModeRef mode = (CGDisplayModeRef) CFArrayGetValueAtIndex(displayModeList, modeIndex);
		
		displayMode.refreshRate = CGDisplayModeGetRefreshRate(mode);
		displayMode.width = CGDisplayModeGetPixelWidth(mode);
		displayMode.height = CGDisplayModeGetPixelHeight(mode);

	}

	CFRelease(displayModeList);
	
	return displayMode;
}

DisplayMode SystemInfo::GetCurrentDisplayMode(const Display& display)
{
	DisplayMode displayMode;
	CGDisplayModeRef mode = CGDisplayCopyDisplayMode(display.id);
	displayMode.refreshRate = CGDisplayModeGetRefreshRate(mode);
	displayMode.width = CGDisplayModeGetPixelWidth(mode);
	displayMode.height = CGDisplayModeGetPixelHeight(mode);
	CFRelease(mode);
	return displayMode;
}

/*
void System::SetDisplayMode(const Display& display, int modeIndex)
{
	if(modeIndex < NumDisplayMode(display))
	{
		
	}
}
*/

Window	WindowManager::CreateWin(const Rect& rect)
{
	CreateWinParam param;
	WindowThreadCmd cmd;
	param.rect = rect;

	param.winCreationCS.Lock();
	cmd.type = WindowThreadCmd::CREATE_WINDOW;
	cmd.createWinParam = &param;

	while (!mgrImpl->winThreadCmdQueue.Enqueue(cmd))
	;

	WaitForCondVar(param.winCreationCV, param.winCreationCS);

	param.winCreationCS.Unlock();

	return param.win;}

void WindowManager::CloseWin(yu::Window &win)
{
	for(int i = 0; i < mgrImpl->windowList.Size(); i++)
	{
		if(win.win == mgrImpl->windowList[i].win)
		{
			[win.win release];
			mgrImpl->windowList.Erase(i);
			break;
		}
	}
}

}