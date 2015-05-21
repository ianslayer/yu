#ifndef YU_LOG_H
#define YU_LOG_H
#include <stdarg.h>

namespace yu
{
struct Logger;
void InitSysLog();
void FreeSysLog();
void VLog(const char* fmt, va_list args);
void Log(const char *fmt, ...);
	
void BeginBlockLog();
void EndBlockLog();
void BlockLog(const char *fmt, ...);

}

#endif
