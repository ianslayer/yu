#include "platform.h"
#if defined YU_OS_WIN32
	#include <malloc.h>
	#include <stdlib.h>
#elif defined YU_OS_MAC
	#include <malloc/malloc.h>
#endif

#include <assert.h>
#include <new>

#include "bit.h"
#include "allocator.h"

namespace yu
{

DefaultAllocator  _gDefaultAllocator;
DefaultAllocator* gDefaultAllocator = nullptr;

	
void* Allocator::AllocAligned(size_t size, size_t align)
{
	assert(IsPowerOfTwo(align));
		
	align = (align > sizeof(size_t)) ? align : sizeof(size_t);
	size_t p = (size_t)Alloc(size+align);
	size_t aligned = 0;
	if (p)
	{
		aligned = (size_t(p) + align-1) & ~(align-1);
		if (aligned == p)
		aligned += align;
		*(((size_t*)aligned)-1) = aligned-p;
	}
	return (void*)aligned;
}
	
void Allocator::FreeAligned(void* ptr)
{
	size_t src = size_t(ptr) - *(((size_t*)ptr)-1);
	Free((void*)src);
}
	
Allocator::~Allocator()
{
		
}
	
void* DefaultAllocator::Alloc(size_t size)
{
	return malloc(size);
}

void* DefaultAllocator::Realloc(void* oldPtr, size_t newSize)
{
	return realloc(oldPtr, newSize);
}

void DefaultAllocator::Free(void* ptr)
{
	free(ptr);
}


void InitDefaultAllocator()
{
	gDefaultAllocator = &_gDefaultAllocator;
}

void FreeDefaultAllocator()
{

}
	
StackAllocator::StackAllocator(size_t _bufferSize, Allocator* baseAllocator)
	: waterBase(0), waterMark(0), bufferSize(_bufferSize), allocator(baseAllocator)
{
	buffer = baseAllocator->Alloc(_bufferSize);
}
	
StackAllocator::~StackAllocator()
{
	Free();
}

void StackAllocator::Free()
{
	allocator->Free(buffer);
}
	
void*	StackAllocator::Alloc(size_t size)
{
	size_t allocSize = (size + sizeof(size_t));
	
	if(waterMark + allocSize > bufferSize)
	{
		return nullptr;
	}
	
	size_t oldWaterBase = waterBase;
	waterBase = waterMark;
	
	waterMark += allocSize;
	
	void* allocBase = (u8*)buffer + waterBase;
	
	*((size_t*)allocBase ) = oldWaterBase;
	
	return (u8*)allocBase+sizeof(size_t);
}
	
void*	StackAllocator::Realloc(void* oldPtr, size_t newSize)
{
	return Alloc(newSize);
}
	
void	StackAllocator::Free(void* ptr)
{
	if(ptr == ((u8*)buffer + waterBase + sizeof(size_t)))
	{
		size_t oldBase = *((size_t*)ptr - 1) ;
		waterMark = waterBase;
		waterBase = oldBase;
	}
}
	
}


void * operator new(size_t n) throw()
{
	void* p = yu::_gDefaultAllocator.Alloc(n);

	if(!p)
	{
		exit(1);
	}

	return p;
}

/*

void * operator new(size_t n) throw(std::bad_alloc)
{
	void* p = yu::_gDefaultAllocator.Alloc(n);

	if(!p)
	{
		//exit(1);
		static const std::bad_alloc nomem;
		throw nomem;
	}

	return p;
}
*/


void * operator new[] (size_t n) throw()
{
	void* p = yu::_gDefaultAllocator.Alloc(n);

	if(!p)
	{
		exit(1);
		
	}

	return p;
}


/*
void * operator new[](size_t n) throw(std::bad_alloc)
{
	void* p = yu::_gDefaultAllocator.Alloc(n);

	if(!p)
	{
		//exit(1);
		static const std::bad_alloc nomem;
		throw nomem;
	}

	return p;
}
*/

void operator delete(void * p) throw()
{
	yu::_gDefaultAllocator.Free(p);
}

void operator delete[](void * p) throw()
{
	yu::_gDefaultAllocator.Free(p);
}

