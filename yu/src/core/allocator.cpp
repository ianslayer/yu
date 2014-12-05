#include "platform.h"
#if defined YU_OS_WIN32
	#include <malloc.h>
	#include <stdlib.h>
#elif defined YU_OS_MAC
	#include <malloc/malloc.h>
#endif

#include <assert.h>

#include "../container/array.h"
#include "bit.h"
#include "allocator.h"
#include "log.h"

namespace yu
{

const static size_t MIN_ALLOC_ALIGN = sizeof(void*);

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
	void* ptr = malloc(size);

	/*
	if ((size_t(ptr) & 0x7) != 0)
	{
		assert((size_t(ptr) & 0x7) == 0);
	}
	*/

	return ptr;
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
	
struct ArenaImpl
{
	ArenaImpl(size_t _blockSize, Allocator* baseAllocator) 
		: allocator(baseAllocator), blocks(16, baseAllocator), blockHeaders(16, baseAllocator), blockSize(_blockSize)
	{
		
	}
	~ArenaImpl()
	{
		for (int i = 0; i <blocks.Size(); i++)
		{
			allocator->Free(blocks[i]);
		}
	}

	struct BlockHeader
	{
		size_t blockSize;
		size_t waterMark;
	};

	Array<void*>		blocks;
	Array<BlockHeader>	blockHeaders;
	Allocator*			allocator;
	size_t				blockSize;
};

ArenaAllocator::ArenaAllocator(size_t blockSize, Allocator* baseAllocator)
{
	arenaImpl = new(baseAllocator->Alloc(sizeof(*arenaImpl))) ArenaImpl(blockSize, baseAllocator);
}

ArenaAllocator::~ArenaAllocator()
{
	Allocator* baseAllocator = arenaImpl->allocator;
	Destruct(arenaImpl);
	baseAllocator->Free(arenaImpl);
}

void* ArenaAllocator::Alloc(size_t size)
{
	size_t allocSize = RoundUp(size + MIN_ALLOC_ALIGN - 1, MIN_ALLOC_ALIGN);
	void* availBlock = nullptr;
	ArenaImpl::BlockHeader* blockHeader = nullptr;

	for (int i = 0; i < arenaImpl->blockHeaders.Size(); i++)
	{
		ArenaImpl::BlockHeader& header = arenaImpl->blockHeaders[i];
		if (header.waterMark + allocSize <= header.blockSize)
		{
			availBlock = arenaImpl->blocks[i];
			blockHeader = &header;
			break;
		}
	}

	if (!availBlock)
	{
		ArenaImpl::BlockHeader newHeader;
		newHeader.blockSize = allocSize>arenaImpl->blockSize ? allocSize : arenaImpl->blockSize;
		newHeader.waterMark = 0;
		void* newBlock = arenaImpl->allocator->Alloc(newHeader.blockSize);
		arenaImpl->blocks.PushBack(newBlock);
		arenaImpl->blockHeaders.PushBack(newHeader);

		availBlock = newBlock;
		blockHeader = arenaImpl->blockHeaders.Rbegin();
	}

	void* p = (void*) RoundUp(size_t(availBlock) + blockHeader->waterMark, MIN_ALLOC_ALIGN);
	blockHeader->waterMark += allocSize;
	return p;
}

void ArenaAllocator::Free(void* p)
{
	Log("warning, try to free memory from arena\n");
}

void* ArenaAllocator::Realloc(void* oldPtr, size_t newSize)
{
	return Alloc(newSize);
}

void ArenaAllocator::RecycleBlocks()
{
	for (int i = 0; i < arenaImpl->blockHeaders.Size(); i++)
	{
		arenaImpl->blockHeaders[i].waterMark = 0;
	}
}

StackAllocator::StackAllocator(size_t _bufferSize, Allocator* baseAllocator)
	: waterBase(0), waterMark(0), bufferSize(_bufferSize), allocator(baseAllocator)
{
	buffer = baseAllocator->Alloc(_bufferSize);
}
	
StackAllocator::~StackAllocator()
{
	allocator->Free(buffer);
}

void StackAllocator::Rewind()
{
	size_t allocBase = (size_t)buffer + waterBase;
	size_t aligned = RoundUp(allocBase + MIN_ALLOC_ALIGN, MIN_ALLOC_ALIGN);

	size_t oldWaterBase = *(((size_t*)aligned) - 1);

	waterMark = waterBase;
	waterBase = oldWaterBase;
}

void StackAllocator::Rewind(size_t oldWaterMark)
{
	assert(oldWaterMark < waterMark);
	size_t allocBase = (size_t)buffer + oldWaterMark;
	size_t aligned = 0;
	aligned = RoundUp(allocBase + MIN_ALLOC_ALIGN, MIN_ALLOC_ALIGN);

	size_t oldWaterBase = *(((size_t*)aligned) - 1);
	waterBase = oldWaterBase;
	waterMark = oldWaterMark;
}

void*	StackAllocator::Alloc(size_t size)
{
	size_t allocSize = RoundUp(size + MIN_ALLOC_ALIGN, MIN_ALLOC_ALIGN);
	
	if(waterMark + allocSize > bufferSize)
	{
		yu::Log("error: stack allocator full\n");
		return nullptr;
	}
	
	size_t oldWaterBase = waterBase;
	waterBase = waterMark;
	
	waterMark += allocSize;
	
	size_t allocBase = (size_t)buffer + waterBase;
	size_t aligned = 0;
	aligned = RoundUp(allocBase + MIN_ALLOC_ALIGN, MIN_ALLOC_ALIGN);

	*(((size_t*)aligned) - 1) = oldWaterBase;
	
	return (void*)aligned;
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
		yu::Log("error: out of memory\n");
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
		yu::Log("error: out of memory\n");
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

