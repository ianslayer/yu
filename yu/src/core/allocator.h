#ifndef YU_ALLOCATOR_H
#define YU_ALLOCATOR_H

namespace yu
{

class Allocator
{
public:
	virtual ~Allocator() = 0;
	virtual void*	Alloc(size_t size) = 0;
	virtual void*	Realloc(void* oldPtr, size_t newSize) = 0;
	virtual void	Free(void* ptr) = 0;

	virtual void*	AllocAligned(size_t size, size_t align);
	virtual void	FreeAligned(void* ptr) ;
};

class DefaultAllocator : public Allocator
{
public:
	virtual void*	Alloc(size_t size);
	virtual void*	Realloc(void* oldPtr, size_t newSize);
	virtual void	Free(void* ptr);

};
extern DefaultAllocator* gDefaultAllocator;

struct ArenaImpl;
class ArenaAllocator : public Allocator
{
public:
	ArenaAllocator(size_t blockSize = 64 * 1024, Allocator* baseAllocator = gDefaultAllocator);
	virtual ~ArenaAllocator();
	virtual void*	Alloc(size_t size);
	//virtual void*	AllocNonAligned(size_t size);//this is crazy
	virtual void*	Realloc(void* oldPtr, size_t newSize);
	virtual void	Free(void* ptr);

	void			RecycleBlocks();

private:
	ArenaImpl* arenaImpl;
};
extern ArenaAllocator* gSysArena;

class VmArenaAllocator : public ArenaAllocator
{

};

class StackAllocator : public Allocator
{
public:
	StackAllocator(size_t bufferSize, Allocator* baseAllocator = gDefaultAllocator);
	virtual ~StackAllocator();
	virtual void*	Alloc(size_t size);
	virtual void*	Realloc(void* oldPtr, size_t newSize);
	virtual void	Free(void* ptr);

	void			Rewind();
	void			Rewind(size_t oldWaterMark);

	void*			buffer;
	Allocator*		allocator;
	size_t			bufferSize;
	size_t			waterBase;
	size_t			waterMark;
};
	
void InitDefaultAllocator();
void FreeDefaultAllocator();

}


#endif