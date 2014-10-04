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
	

class StackAllocator : public Allocator
{
public:
	StackAllocator(size_t bufferSize, Allocator* baseAllocator = gDefaultAllocator);
	virtual ~StackAllocator();
	virtual void*	Alloc(size_t size);
	virtual void*	Realloc(void* oldPtr, size_t newSize);
	virtual void	Free(void* ptr);


	void			Free();

	void*			buffer;
	Allocator*		allocator;
	size_t			bufferSize;
	size_t			waterBase;
	size_t			waterMark;
};

class StaticAllocator : public Allocator
{
	StaticAllocator(size_t bufferSize, Allocator* baseAllocator = gDefaultAllocator);
	virtual ~StaticAllocator();
	virtual void*	Alloc(size_t size);
	virtual void*	Realloc(void* oldPtr, size_t newSize);
	virtual void	Free(void* ptr);

	void			Free();
	
	void*			buffer;
	Allocator*		allocator;
	size_t			bufferSize;
	size_t			waterMark;
};
	
void InitDefaultAllocator();
void FreeDefaultAllocator();


}

#endif