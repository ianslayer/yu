#ifndef YU_H
#define YU_H

namespace yu
{
void InitSysLog();
void FreeSysLog();
void InitSysAllocator();
void FreeSysAllocator();
void MainThreadWorker();
void InitWorkerSystem();
void FreeWorkerSystem();
void SubmitTerminateWork();
void InitThreadRuntime();
void FreeThreadRuntime();
bool AllThreadsExited();
void InitYu();
bool Initialized();
void FreeYu();
int YuMain();
}

#endif
