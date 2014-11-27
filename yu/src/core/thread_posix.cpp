#include "thread.h"
#include "log.h"
#include <sys/types.h>
#include <sys/sysctl.h>

namespace yu
{

void RegisterThread(ThreadHandle thread);
void ThreadExit(ThreadHandle handle);

struct ThreadRunnerContext
{
	ThreadPriority	priority;
	u64				affinityMask;
	ThreadFunc		func;
	ThreadContext	context;

	CondVar			threadCreationCV;
	Mutex			threadCreationCS;
};

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

	pthread_create(&thread.threadHandle, nullptr, ThreadRunner, &runnerContext);
	WaitForCondVar(runnerContext.threadCreationCV, runnerContext.threadCreationCS);
	runnerContext.threadCreationCS.Unlock();

	return thread;
}

ThreadHandle GetCurrentThreadHandle()
{
	return pthread_self();
}

#if defined YU_OS_MAC
void	SetThreadAffinity(ThreadHandle threadHandle, u64 affinityMask)
{

	int mib[2];
	size_t len=1024;
	char p[1024];

	mib[0] = CTL_HW;
	mib[1] = HW_PAGESIZE;
	//sysctl(mib, 2, p, &len, NULL, 0);
	sysctlbyname("hw.physicalcpu_max", p, &len, NULL, 0);
	
	thread_act_t thread = pthread_mach_thread_np(threadHandle);

	if(affinityMask == 0) return;
	
	thread_affinity_policy affinityTag[64];
	memset(affinityTag, sizeof(affinityTag), 0);
	
	unsigned int numCore = 0;
	
	for(int i = 0; i < 64; i++)
	{
		if((affinityMask>>i) & 1)
		{
			affinityTag[numCore].affinity_tag = i;
			numCore++;
		}
	}
	
	kern_return_t ret = thread_policy_set(thread, THREAD_AFFINITY_POLICY, (thread_policy_t) affinityTag, numCore);
	
	if(ret != KERN_SUCCESS)
	{
		Log("error: thread affinity set failed\n");
	}
	
}
#endif

Mutex::Mutex()
{
	pthread_mutex_init(&m, nullptr);
}

Mutex::~Mutex()
{
	pthread_mutex_destroy(&m);
}

void Mutex::Lock()
{
	pthread_mutex_lock(&m);
}

void Mutex::Unlock()
{
	pthread_mutex_unlock(&m);
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
	pthread_cond_init(&cv, nullptr);
}

CondVar::~CondVar()
{
	pthread_cond_destroy(&cv);
}

void WaitForCondVar(CondVar& cv, Mutex& cs)
{
	pthread_cond_wait(&cv.cv, &cs.m);
}

void NotifyCondVar(CondVar& cv)
{
	pthread_cond_signal(&cv.cv);
}

void NotifyAllCondVar(CondVar& cv)
{
	pthread_cond_broadcast(&cv.cv);
}

}
