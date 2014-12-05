#ifndef YU_LOG_H
#define YU_LOG_H

namespace yu
{
class Logger;
void InitSysLog();
void FreeSysLog();
void Log(char const *fmt, ...);

void BeginBlockLog();
void EndBlockLog();
void BlockLog(char const *fmt, ...);

}

#endif
