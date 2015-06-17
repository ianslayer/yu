#ifndef YU_LOG_H
#define YU_LOG_H
#include <stdarg.h>

namespace yu
{
struct Logger;
void InitSysLog();
void FreeSysLog();

enum LogFilter
{
	LOG_ERROR,
	LOG_WARNING,
	LOG_INFO,
	LOG_MESSAGE,
};
	
void VLog(const char* fmt, va_list args);
void VFilterLog(int filter, const char* fmt, va_list args);
void Log(const char *fmt, ...);
void FilterLog(int filter, const char* fmt, ...);
void SetLogFilter(int filter);
	
void BeginBlockLog();
void EndBlockLog();
void BlockLog(const char *fmt, ...);

}

#endif
