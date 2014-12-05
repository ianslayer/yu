#include "thread_impl.h"

namespace yu
{

struct ThreadRunnerContext
{
	ThreadPriority	priority;
	u64				affinityMask;
	ThreadFunc*		func;
	ThreadContext	context;

	CondVar			threadCreationCV;
	Mutex			threadCreationCS;
};
void RegisterThread(ThreadHandle thread);
void ThreadExit(ThreadHandle handle);

ThreadHandle GetCurrentThreadHandle()
{
	DWORD threadId = GetThreadId(GetCurrentThread());

	return threadId;

	/*
	DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &threadHandle,
		DUPLICATE_SAME_ACCESS, FALSE, DUPLICATE_SAME_ACCESS);
		*/
}

ThreadReturn ThreadCall ThreadRunner(ThreadContext context)
{
	ThreadRunnerContext* runnerContext = (ThreadRunnerContext*)context;
	runnerContext->threadCreationCS.Lock();

	ThreadHandle threadHandle= GetCurrentThreadHandle();
	SetThreadAffinity(threadHandle, runnerContext->affinityMask);
	RegisterThread(threadHandle);

	//subtle, but this two must be copied before notify,, else runnerContext my be destroied before they can be executed
	ThreadFunc* func = runnerContext->func;
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
	::CreateThread(NULL, 0, ThreadRunner, &runnerContext, 0, &threadId);
	thread.handle = threadId;

	WaitForCondVar(runnerContext.threadCreationCV, runnerContext.threadCreationCS);
	runnerContext.threadCreationCS.Unlock();

	return thread;
}

u64 GetThreadAffinity(ThreadHandle threadHandle)
{
	GROUP_AFFINITY groupAffinity = {};
	HANDLE realHandle = OpenThread(THREAD_ALL_ACCESS, FALSE, threadHandle);
	if (realHandle)
	{
		::GetThreadGroupAffinity(realHandle, &groupAffinity);
		CloseHandle(realHandle);
		return groupAffinity.Mask;
	}
	return 0;
}

void SetThreadAffinity(ThreadHandle threadHandle, u64 affinityMask)
{
	if (affinityMask)
	{
		HANDLE realHandle = OpenThread(THREAD_ALL_ACCESS, FALSE, threadHandle);
		if (realHandle)
		{
			::SetThreadAffinityMask(realHandle, affinityMask);
			CloseHandle(realHandle);
		}
	}
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
	info.dwThreadID = threadHandle;
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
	InitializeCriticalSectionAndSpinCount(&m, 1000);
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

Semaphore::Semaphore(int initCount, int maxCount)
{
	sem = CreateSemaphore(nullptr, initCount, maxCount, nullptr);
}

Semaphore::~Semaphore()
{
	CloseHandle(sem);
}

void WaitForSem(Semaphore& sem)
{
	WaitForSingleObject(sem.sem, INFINITE);
}

void SignalSem(Semaphore& sem)
{
	LONG prevCount;
	ReleaseSemaphore(sem.sem, 1, &prevCount);
}

}