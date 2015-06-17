#ifndef YU_THREAD_H
#define YU_THREAD_H

#include "platform.h"
#if defined YU_OS_WIN32
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <windows.h>
#elif defined YU_OS_MAC
	#include <mach/mach.h>
	#include <pthread.h>
	#include <semaphore.h>
#endif

#include "yu_lib.h"

#if defined YU_CC_MSVC
#	define YU_THREAD_LOCAL __declspec(thread)
#elif defined (YU_CC_CLANG) || defined (YU_CC_GNU)
#	define YU_THREAD_LOCAL __thread
#endif

#include <atomic>

namespace yu
{

#if defined YU_OS_WIN32
	typedef DWORD ThreadReturn;
	typedef PVOID ThreadContext;
	#define ThreadCall WINAPI
	typedef DWORD ThreadHandle;
#else
	typedef void* ThreadReturn;
	typedef void* ThreadContext;
	#define ThreadCall 
	typedef pthread_t ThreadHandle;
#endif

typedef ThreadReturn ThreadCall ThreadFunc(ThreadContext);

enum ThreadPriority
{
	CriticalPriority,
	HighestPriority,
	AboveNormalPriority,
	NormalPriority,
	BelowNormalPriority,
	LowestPriority,
	IdlePriority,
};

struct Thread
{
	ThreadHandle handle;
};

class Allocator;
void			InitThreadRuntime(Allocator* defaultAllocator);
void			FreeThreadRuntime(Allocator* defaultAllocator);
Thread			CreateThread(ThreadFunc func, ThreadContext context, ThreadPriority priority = NormalPriority, u64 affinityMask = 0);
ThreadHandle	GetCurrentThreadHandle();
u64				GetThreadAffinity(ThreadHandle thread);
void			SetThreadAffinity(ThreadHandle threadHandle, u64 affinityMask);
void			SetThreadName(ThreadHandle thread, const char* name);
void			SetThreadPriority(ThreadHandle thread, ThreadPriority priority);
bool			AllThreadsExited();
unsigned int	NumThreads();

struct			FrameLock;
FrameLock*		AddFrameLock();
void			WaitForKick(FrameLock* lock);
void			FrameComplete(FrameLock* lock);
u64				FrameCount();

class Allocator;
struct ThreadContextStackEntry
{
	 Allocator* allocator;	
};

const ThreadContextStackEntry* GetCurrentThreadStackEntry();
void PushThreadContext(const ThreadContextStackEntry& entry);
ThreadContextStackEntry PopThreadContext();

}

#endif
