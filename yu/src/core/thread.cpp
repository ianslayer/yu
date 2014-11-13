#include "thread.h"
namespace yu
{

struct ThreadRunnerContext
{
	ThreadPriority	priority;
	u64				affinityMask;
	ThreadFunc		func;
	ThreadContext	context;
};

ThreadReturn ThreadCall ThreadRunner(ThreadContext context)
{
	ThreadRunnerContext* runnderContext = (ThreadRunnerContext*)context;
	//set affinity, priority here
	ThreadReturn ret;
		
	ret = runnderContext->func(runnderContext->context);

	return ret;
}

Thread CreateThread(ThreadFunc func, ThreadContext context, ThreadPriority priority, u64 affinityMask)
{
	Thread thread;

	ThreadRunnerContext runnerContext;
	runnerContext.priority = priority;
	runnerContext.affinityMask = affinityMask;
	runnerContext.func = func;
	runnerContext.context = context;

	thread.hThread = ::CreateThread(NULL, 0, ThreadRunner, &runnerContext, 0, &thread.threadId);
	return thread;
}

/*
u64 GetThreadAffinity(Thread& thread)
{
	
}
*/

void SetThreadAffinity(Thread& thread, u64 affinityMask)
{
	::SetThreadAffinityMask(thread.hThread, affinityMask);
}

}