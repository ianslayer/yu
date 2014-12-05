#include "platform.h"
#include "string.h"
#include "thread.h"
#include <stdio.h>
#include <stdarg.h>

namespace yu
{
#define BUF_LEN 16 * 4096

struct Logger
{
	char	logBuffer[BUF_LEN];
	size_t	endChar;
	Mutex	m;
	FILE*	logFile;
};

Logger* gLogger;

void InitSysLog()
{
	gLogger = new Logger;
}

void FreeSysLog()
{
	//flush here
	delete gLogger;
}

void Log(Logger* logger, char const *fmt,  va_list args)
{
	ScopedLock lock(logger->m);
	char buf[1024] = {};

#if defined YU_OS_WIN32
	vsnprintf_s(buf, 1024, 1023, fmt, args);
#else
	vsnprintf(buf, 1024, fmt, args);
#endif


	size_t logLength = strlen(buf);

	//#if defined YU_OS_WIN32
	//OutputDebugStringA(buf);
	//#else
	printf(buf);
	//#endif

	if (BUF_LEN < logger->endChar + logLength)
	{
		//flush buffer here
		logger->endChar = 0;
	}

	strncpy(logger->logBuffer + logger->endChar, buf, logLength);
	logger->endChar += logLength;
}

void Log(char const *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	Log(gLogger, fmt, args);
	va_end(args);
}

}