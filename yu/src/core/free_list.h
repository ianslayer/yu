#ifndef YU_FREE_LIST_H
#define YU_FREE_LIST_H
#include <atomic>

namespace yu
{

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

		return freeObject[allocIdx];
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