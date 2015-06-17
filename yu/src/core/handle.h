#ifndef YU_HANDLE
#define YU_HANDLE

#include "yu_lib.h"

namespace yu
{
	
struct Handle
{
	i32 id;
	i32 seq;
};

inline bool operator == (const Handle& h0, const Handle& h1)
{
	return (h0.id == h1.id) && (h0.seq == h1.seq);
}

inline Handle InvalidHandle()
{
	Handle invalidHandle = {-1, -1};
	return invalidHandle;
}

//TODO: see http://cbloomrants.blogspot.tw/2013/05/05-08-13-lock-free-weak-reference-table.html

template<class ObjType>
struct HandleTable
{
	
};

template<class DerivedType>
struct BaseObject
{
	HandleTable<DerivedType>*	table;
	std::atomic<i32>			refCount;
};

template<int num>
struct HandleFreeList
{
	HandleFreeList();
	
	Handle	Alloc();
	void	Free(Handle handle);
	
	int		numAlloced;
	Handle	handleList[num];

	DEBUG_ONLY(Handle allocedList[num]);	
	Mutex	m;
};

template<int num>
HandleFreeList<num>::HandleFreeList() : numAlloced(0)
{
	for(int i = 0; i < num; i++)
	{
		handleList[i].id = i;
		handleList[i].seq = -1;
		DEBUG_ONLY(allocedList[i] = InvalidHandle() );
	}
}

template<int num>
Handle HandleFreeList<num>::Alloc()
{
	ScopedLock lock(m);
	int allocIndex = numAlloced++;
	assert(allocIndex < num);

	handleList[allocIndex].seq++;
   	Handle allocHandle = handleList[allocIndex];

	DEBUG_ONLY(allocedList[allocIndex] = allocHandle);
	
	return allocHandle;
}

template<int num>	
void HandleFreeList<num>::Free(Handle handle)
{
	ScopedLock lock(m);

	assert(numAlloced > 0);
#if defined YU_DEBUG
	for(int i = 0; i < numAlloced; i++)
	{
		if(allocedList[i] == handle)
		{
			allocedList[i] = allocedList[numAlloced - 1];
			allocedList[numAlloced - 1] = InvalidHandle();
		}
	}
#endif

	handleList[--numAlloced] = handle;
}

template<class HandleType> inline HandleType ToTypedHandle(Handle handle)
{
	HandleType typedHandle;
	typedHandle.id = handle.id;
	typedHandle.seq = handle.seq;
	return typedHandle;
}

template<class HandleType> inline Handle ToHandle(HandleType typedHandle)
{
	Handle handle;
	handle.id = typedHandle.id;
	handle.seq = typedHandle.seq;
	return handle;
}
	
}

#endif
