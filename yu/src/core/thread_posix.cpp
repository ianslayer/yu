#include "platform.h"
#include <new>
#include "allocator.h"
#include "thread_impl.h"
#include "string.h"
#include "log.h"

namespace yu
{

void RegisterThread(ThreadHandle thread);
void ThreadExit(ThreadHandle handle);

struct ThreadRunnerContext
{
	ThreadPriority	priority;
	u64				affinityMask;
	ThreadFunc*		func;
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

	pthread_create(&thread.handle, nullptr, ThreadRunner, &runnerContext);
	WaitForCondVar(runnerContext.threadCreationCV, runnerContext.threadCreationCS);
	runnerContext.threadCreationCS.Unlock();

	return thread;
}

ThreadHandle GetCurrentThreadHandle()
{
	return pthread_self();
}

#if defined YU_OS_MAC

u64	GetThreadAffinity(ThreadHandle threadHandle)
{
	thread_act_t thread = pthread_mach_thread_np(threadHandle);
	thread_affinity_policy affinityTag[64];
	yu::memset(affinityTag, sizeof(affinityTag), 0);
	unsigned int infoCount = 64;
	boolean_t getDefault = false;
	kern_return_t ret = thread_policy_get(thread, THREAD_AFFINITY_POLICY, (thread_policy_t) affinityTag, &infoCount, &getDefault);

	if(ret != KERN_SUCCESS)
	{
		Log("error: get thread affinity failed\n");
		return 0;
	}
	
	u64 mask = 0;
	for(unsigned int i = 0; i < infoCount; i++)
	{
		if(affinityTag[i].affinity_tag > 0)
			mask |= (1 << (affinityTag[i].affinity_tag-1));
	}
	
	return mask;
}

void	SetThreadAffinity(ThreadHandle threadHandle, u64 affinityMask)
{
	
	thread_act_t thread = pthread_mach_thread_np(threadHandle);

	if(affinityMask == 0) return;
	
	thread_affinity_policy affinityTag[64];
	yu::memset(affinityTag, sizeof(affinityTag), 0);
	
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

static char yuSemName[128];
static std::atomic<unsigned int> semName;
Semaphore::Semaphore(int initCount, int maxCount)
{
	unsigned int name = semName.fetch_add(1);
	StringBuilder nameStr(yuSemName, sizeof(yuSemName));
	nameStr.Cat("yuSem");
	nameStr.Cat(name);
	sem = sem_open(yuSemName, O_CREAT, 0644, (unsigned int)initCount);
}

Semaphore::~Semaphore()
{
	sem_close(sem);
}

void WaitForSem(Semaphore& sem)
{
	sem_wait(sem.sem);
}

void SignalSem(Semaphore& sem)
{
	sem_post(sem.sem);
}

}
