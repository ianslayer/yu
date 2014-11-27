#ifndef YU_LOG_H
#define YU_LOG_H

namespace yu
{
void InitLog();
void FreeLog();
void LockLog();
void UnlockLog();
void Log(char const *fmt, ...);
void LLog(char const *fmt, ...);
}

#endif
