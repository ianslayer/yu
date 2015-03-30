#import "yu_app.h"
#include "../core/system.h"
#include "renderer.h"
#include "gl_utility.h"

void InitGLContext(const yu::Window& win, const yu::RendererDesc& desc)
{
	YuView* yuView = [win.win contentView];
	[yuView initOpenGL];
}

void SwapBuffer(yu::Window& win)
{
	YuView* yuView = [win.win contentView];
	[[yuView openGLContext] flushBuffer];
}