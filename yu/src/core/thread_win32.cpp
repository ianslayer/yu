#include "thread.h"
#include "timer.h"
namespace yu
{

struct ThreadRunnerContext
{
	ThreadPriority	priority;
	u64				affinityMask;
	ThreadFunc		func;
	ThreadContext	context;

	CondVar			threadCreationCV;
	Mutex			threadCreationCS;
};
void RegisterThread(ThreadHandle thread);
void ThreadExit(ThreadHandle handle);

ThreadHandle GetCurrentThreadHandle()
{
	ThreadHandle threadHandle = GetCurrentThread();

	DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &threadHandle,
		DUPLICATE_SAME_ACCESS, FALSE, DUPLICATE_SAME_ACCESS);

	return threadHandle;
}

ThreadReturn ThreadCall ThreadRunner(ThreadContext context)
{
	ThreadRunnerContext* runnerContext = (ThreadRunnerContext*)context;
	runnerContext->threadCreationCS.Lock();

	ThreadHandle threadHandle= GetCurrentThreadHandle();
	SetThreadAffinity(threadHandle, runnerContext->affinityMask);
	RegisterThread(threadHandle);

	//subtle, but this two must be copied before notify,, else runnerContext my be destroied before they can be executed
	ThreadFunc func = runnerContext->func;
	ThreadContext threadContext = runnerContext->context;

	runnerContext->threadCreationCS.Unlock();
	NotifyCondVar(runnerContext->threadCreationCV);

	ThreadReturn ret;
	ret = func(threadContext);

	ThreadExit(threadHandle);

	return ret;
}

Thread CreateThread(ThreadFunc func, ThreadContext context, ThreadPriority priority, u64 affinityMask)
{
	Thread thread;

	ThreadRunnerContext runnerContext;
	runnerContext.threadCreationCS.Lock();
	runnerContext.priority = priority;
	runnerContext.affinityMask = affinityMask;
	runnerContext.func = func;
	runnerContext.context = context;

	DWORD threadId;
	thread.threadHandle= ::CreateThread(NULL, 0, ThreadRunner, &runnerContext, 0, &threadId);
	WaitForCondVar(runnerContext.threadCreationCV, runnerContext.threadCreationCS);
	runnerContext.threadCreationCS.Unlock();

	return thread;
}

/*
u64 GetThreadAffinity(Thread& thread)
{
	
}
*/

void SetThreadAffinity(ThreadHandle threadHandle, u64 affinityMask)
{
	if (affinityMask)
		::SetThreadAffinityMask(threadHandle, affinityMask);
}

const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // Must be 0x1000.
	LPCSTR szName; // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1=caller thread).
	DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void SetThreadName(ThreadHandle threadHandle, const char* name)
{
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = name;
	info.dwThreadID = GetThreadId(threadHandle);
	info.dwFlags = 0;

	__try
	{
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
}

Mutex::Mutex()
{
	InitializeCriticalSection(&m);
}

Mutex::~Mutex()
{

}

void Mutex::Lock()
{
	EnterCriticalSection(&m);
}

void Mutex::Unlock()
{
	LeaveCriticalSection(&m);
}

ScopedLock::ScopedLock(Mutex& _m) : m(_m)
{
	m.Lock();
}

ScopedLock::~ScopedLock()
{
	m.Unlock();
}

CondVar::CondVar()
{
	InitializeConditionVariable(&cv);
}

CondVar::~CondVar()
{

}

void WaitForCondVar(CondVar& cv, Mutex& m)
{
	SleepConditionVariableCS(&cv.cv, &m.m, INFINITE);
}

void NotifyCondVar(CondVar& cv)
{
	WakeConditionVariable(&cv.cv);
}

void NotifyAllCondVar(CondVar& cv)
{
	WakeAllConditionVariable(&cv.cv);
}

}