#include "core/platform.h"
#include "core/refcount.h"
#include "core/timer.h"
#include "core/system.h"
#include "container/array.h"
#include "container/hash.h"

#include <stdio.h>


struct IntHash
{
	int operator()(int x)
	{
		return x;
	}
};

int main()
{
	yu::InitDefaultAllocator();
	yu::InitSysTime();
	yu::InitSystem();

	yu::u64 cpuFreq = yu::EstimateCPUFrequency();

	printf("cpu freq: %llu\n", cpuFreq);
	yu::Array<int> intArray(10);

	yu::Timer timer;
	timer.Start();
	timer.Finish();
	printf("empty time mesuare: %llu\n", timer.cycleCounter.cycle);

	yu::HashTable<int, int, IntHash> hashTab;

	for(int i = 0;i < 20; i++)
	{
		intArray.PushBack(i);
	}

	yu::Array<int> intArray2(intArray);

	hashTab.Set(10, 10);
	hashTab.Remove(10);

	int* x = new int;
	return 0;
}