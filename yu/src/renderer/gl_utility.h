#ifndef YU_GL_UTIL_H
#define YU_GL_UTIL_H

#include "../core/platform.h"

#if defined YU_OS_MAC
    #include <OpenGL/gl3.h>
    #include <OpenGL/gl3ext.h>
    #include <OpenGL/OpenGL.h>

	#if defined __OBJC__
		@class NSOpenGLContext;
	#else
		typedef void NSOpenGLContext;
	#endif

#endif

namespace yu
{

struct GLContext
{
#if defined YU_OS_WIN32

#elif defined YU_OS_MAC
	NSOpenGLContext* context;
#endif
};

}

#endif
