#include <stdio.h>
#include <stdarg.h>
#include "log.h"

namespace yu
{
#define BUF_LEN 16 * 4096

struct Logger
{
	char	logBuffer[BUF_LEN];
	size_t	endChar;
	Mutex	m;
	FILE*	logFile;
	int		 filter;
};

Logger gLogger;

void InitSysLog()
{
	gLogger.filter = LOG_ERROR;
}

void FreeSysLog()
{
	//flush here
	//delete gLogger;
}

void SetLogFilter(Logger* logger, int filter)
{
   	ScopedLock lock(logger->m);
	logger->filter = filter;
}

void SetLogFilter(int filter)
{
	SetLogFilter(&gLogger, filter);
}
	
void Log(Logger* logger, int filter, char const *fmt,  va_list args)
{
	ScopedLock lock(logger->m);
	char buf[1024] = {};

#if defined YU_OS_WIN32
	vsnprintf_s(buf, 1024, 1023, fmt, args);
#else
	vsnprintf(buf, 1024, fmt, args);
#endif


	size_t logLength = strlen(buf);

	if(filter <= logger->filter)
	{
		//#if defined YU_OS_WIN32
		//OutputDebugStringA(buf);
		//#else
		printf(buf);
		//#endif
	}
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
	Log(&gLogger, LOG_ERROR, fmt, args);
	va_end(args);
}

void FilterLog(int filter, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	Log(&gLogger, filter, fmt, args);
	va_end(args);
}
	
void VLog(const char* fmt, va_list args)
{
	Log(&gLogger, LOG_ERROR, fmt, args);
}

void VFilterLog(int filter, const char* fmt, va_list args)
{
	Log(&gLogger, filter, fmt, args);
}

}
