#ifndef YU_ALLOCATOR_H
#define YU_ALLOCATOR_H

namespace yu
{

class Allocator
{
public:
	virtual ~Allocator() = 0 {}
	virtual void*	Alloc(size_t size) = 0;
	virtual void*	Realloc(void* oldPtr, size_t newSize) = 0;
	virtual void	Free(void* ptr) = 0;

	virtual void*	AllocAligned(size_t size, size_t align) = 0;
	virtual void	FreeAligned(void* ptr) = 0;
};

class DefaultAllocator : public Allocator
{
public:
	virtual void*	Alloc(size_t size);
	virtual void*	Realloc(void* oldPtr, size_t newSize);
	virtual void	Free(void* ptr);

	virtual void*	AllocAligned(size_t size, size_t align);
	virtual void	FreeAligned(void* ptr);
};

//static buffer allocator
//allocated once, then never free until program shutdown
class StaticBufferAllocator : public Allocator 
{
public:
	StaticBufferAllocator(size_t poolSize);
	virtual ~StaticBufferAllocator();
	virtual void*	Alloc(size_t size);
	virtual void*	Realloc(void* oldPtr, size_t newSize); //invalid call
	virtual void	Free(void* ptr);						//invalid call

	virtual void*	AllocAligned(size_t size, size_t align);
	virtual void	FreeAligned(void* ptr);					//invalid call

	void			Free();

	void*			buffer;
	size_t			waterMark;
};

void InitDefaultAllocator();
void FreeDefaultAllocator();
extern DefaultAllocator* gDefaultAllocator;

}

#endif