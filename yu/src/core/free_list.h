#ifndef YU_FREE_LIST_H
#define YU_FREE_LIST_H
#include <atomic>
#include "../container/array.h"
namespace yu
{

	/*
struct IdFreeList
{
	IdFreeList(int initSize, Allocator* allocator) : freeIdList(initSize, allocator), deferredFreeIdList(initSize, allocator)
	{
		for(int i = 0; i < initSize; i++)
		{
			freeIdList.PushBack(i);
		}
		maxId = initSize - 1;
		numAlloced = 0;
	}

	int Alloc()
	{
		int allocIdx = numAlloced.fetch_add(1, std::memory_order_acquire);

		//ensure enough space
		if(allocIdx > maxId )
		{
			ScopedLock lock (m);

			int newMinId = maxId + 1;
			int expandSize = 16;

			int expandedMaxId = newMinId + expandSize;

			for(int i = newMinId; i < expandedMaxId; i++)
			{
				freeIdList.PushBack(i);
			}
		}
		
		int id = freeIdList[allocIdx]; //dangerous
		return id;
	}
   
	Array<int> freeIdList;
	Array<int> deferredFreeIdList;
	std::atomic<int> numAlloced;
	std::atomic<int> maxId;
	Mutex m;
};
	*/
	
template <int num>
struct IndexFreeList
{
	IndexFreeList() : numAlloced(0), numDeferredFreed(0)
	{
		for (int i = 0; i < num; i++)
			indices[i] = i;
	}
	
	int Alloc() //multi thread-safe
	{
		int allocIdx = numAlloced.fetch_add(1, std::memory_order_acquire);
		assert(allocIdx < num);

		if (allocIdx >= num)
			return -1;

		int id = indices[allocIdx];
#if defined YU_DEBUG
		indices[allocIdx] = -1;
#endif
		return id;
	}

	void DeferredFree(int id)
	{
		int freeIdx = numDeferredFreed.fetch_add(1, std::memory_order_acquire);
		assert(freeIdx < num);

		if (freeIdx >= num)
			return;

		deferredFreeIndices[freeIdx] = id;
	}

	void Free() //must executed on one thread in thread safe region
	{
		int numFreedId = numDeferredFreed;
		
		int endFreeIdx = numAlloced.load();
		int StartFreeIdx = endFreeIdx - numFreedId;

		for (int i = 0; i < numFreedId; i++)
		{
			indices[i + StartFreeIdx] = deferredFreeIndices[i];
		}
		 numDeferredFreed = 0;
	}

	int Available()
	{
		return num - numAlloced;
	}

	std::atomic<int> numAlloced;
	int	indices[num];
	int deferredFreeIndices[num];
	std::atomic<int> numDeferredFreed;
};

}

#endif
