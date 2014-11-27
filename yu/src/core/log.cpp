#include "platform.h"
#include "string.h"
#include "thread.h"
#include <stdio.h>
#include <stdarg.h>

namespace yu
{
#define BUF_LEN 16 * 4096
static char gLogBuffer[BUF_LEN];
static size_t gEndChar;
static Mutex* m;

static FILE* gLogFile;

void InitLog()
{
	m = new Mutex;
}

void FreeLog()
{
	//flush here
	delete m;
}

void Log(char const *fmt, ...)
{
	ScopedLock lock(*m);
	char buf[1024]={};
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, 1024, fmt, args);
	va_end (args);
	
	size_t logLength = strlen(buf);
	
	printf(buf);
	
	if( BUF_LEN < gEndChar + logLength)
	{
		//flush buffer here
		gEndChar = 0;
	}
	
	strncpy(gLogBuffer + gEndChar, buf, logLength);
	gEndChar+=logLength;
}

}