#ifndef YU_FREE_LIST_H
#define YU_FREE_LIST_H
#include <atomic>

namespace yu
{

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

		deferredFreeInices[freeIdx] = id;
	}

	void Free() //must executed on one thread in thread safe region
	{
		int numFreedId = numDeferredFreed.load(std::memory_order_acquire);
		
//		int EndFreeIdx = numAlloced.load();
		int StartFreeIdx = EndFreeIdx - numFreedId;

		for (int i = 0; i < numFreedId; i++)
		{
			indices[i + StartFreeIdx] = deferredFreeInices[i];
		}
	}

	int Available()
	{
		return num - numAlloced;
	}

	std::atomic<int> numAlloced;
	int	indices[num];
	int deferredFreeInices[num];
	std::atomic<int> numDeferredFreed;
};

template<class T, int num> struct FreeList
{
	FreeList() : numAlloced(0)
	{
		for (int i = 0; i < num; i++)
			freeObject[i] = i;
	}

	int Alloc()
	{
		int allocIdx = numAlloced.fetch_add(1, std::memory_order_acquire);
		assert(allocIdx < num);

		if (allocIdx >= num)
			return -1;

		int allocObj = freeObject[allocIdx];
#if defined YU_DEBUG
		freeObject[allocIdx] = -1;
#endif
		return allocObj;
	}

	void Free(int index)
	{
		assert((index >= 0) && (index < num));
		int freeIdx = numAlloced.fetch_sub(1, std::memory_order_acquire);
		freeIdx--;
		assert((freeIdx >= 0) && (freeIdx < num));
		freeObject[freeIdx] = index;
	}

	T* Get(int index)
	{
		assert((index >= 0) && (index < num));
		return &object[index];
	}

	const T* Get(int index) const
	{
		assert((index >= 0) && (index < num));
		return &object[index];
	}

	int Available()
	{
		return num - numAlloced;
	}

	int	freeObject[num];
	T	object[num];
	std::atomic<int> numAlloced;
};


}

#endif