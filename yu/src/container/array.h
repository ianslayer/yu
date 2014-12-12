#ifndef YU_ARRAY_H
#define YU_ARRAY_H
#include "../core/type.h"
#include "../core/bit.h"
#include "../core/allocator.h"
#include <assert.h>

namespace yu
{

template <class T>
class Array
{
static const int DEFAULT_CAPACITY = 16;
static const int DEFAULT_GRANULARITY = 16; //we assume granularity is multiple of power of 2

public:
	Array (Allocator* _allocator = gDefaultAllocator);
	Array (int capacity, Allocator* _allocator = gDefaultAllocator);
	Array (int capacity, const T* pData, int size, Allocator* _allocator = gDefaultAllocator);
	Array (const Array& rhs, Allocator* _allocator = gDefaultAllocator);
	~Array();

   void			    PushBack(const T& val);
   int			    Append(const T& val);
   void			    PushBack();
   void			    PopBack();
   
   void			    Erase(T* pAt);
   void			    Erase(int index);
   void			    EraseSwapBack(T* pAt);
   
   void			    Reserve(int capacity);
   void			    Resize(int capacity);
   void			    Condense();
	
   int			    Size() const;
   int			    Capacity() const;

   void			    Clear();
   void			    ClearMem();

   operator         const T*() const;
   operator         T*(); 
	
   const T&         operator[] (int index) const;
   T&               operator[] (int index);

   Array&           operator = (const Array<T>& rhs);

   T*               Begin();
   const T*         Begin() const;
   T*               End();
   const T*         End() const;

   T*               Rbegin();
   const T*         Rbegin() const;

   const T*         Find(const T& val) const; 

   T*               Find(const T& val);

   int              FindIndex(const T& val) const;

   void             SetGranularity(int granularity);

   Allocator*		GetAllocator() { return allocator; }

private:
   void             EnsureCapacity(int newSize);

   T*               pArray;
   Allocator*		allocator;
   int              mCapacity;
   int              mSize;
   int              mGranularity;
};

template<class T> 
Array<T>::Array(Allocator* _allocator) : pArray(0), mCapacity(0), mSize(0), mGranularity(Array::DEFAULT_GRANULARITY), allocator(_allocator)
{

}

template<class T>
Array<T>::Array(int capacity, Allocator* _allocator) : pArray(0), mCapacity(0), mSize(0), mGranularity(Array::DEFAULT_GRANULARITY), allocator(_allocator)
{
	if(capacity > 0)
	{
		mCapacity = (capacity + (DEFAULT_GRANULARITY - 1)) & (-DEFAULT_GRANULARITY);
		pArray = (T*) allocator->Alloc(sizeof(T) * (size_t)mCapacity);//new T[mCapacity];
	}
	else
	{
		mCapacity = 0;
		pArray = NULL;
	}

}


template<class T>
Array<T>::Array(int capacity, const T* pData, int size, Allocator* _allocator) : pArray(0), mCapacity(0), mSize(0), mGranularity(Array::DEFAULT_GRANULARITY), allocator(_allocator)
{
	assert(capacity >= size);

	mCapacity = (capacity + (mGranularity - 1)) & (-mGranularity);
	pArray = (T*) allocator->Alloc(sizeof(T) * mCapacity); //new T[mCapacity];
	Construct(pArray, size);
	copy_n(pData, size, pArray);
	mSize = size;

}

template<class T>
Array<T>::Array(const Array<T> &rhs, Allocator* _allocator) : pArray(0), mCapacity(rhs.mCapacity), mSize(rhs.mSize), mGranularity(rhs.mGranularity), allocator(_allocator)
{
	pArray = (T*) allocator->Alloc(sizeof(T) * mCapacity);
	Construct(pArray, mSize);
	copy_n(rhs.Begin(), rhs.Size(), pArray);
}

template<class T>
Array<T>& Array<T>::operator=(const Array<T>& rhs)
{
	if(this == &rhs)
		return *this;
	
	Clear();
	mGranularity = rhs.mGranularity;
	EnsureCapacity(rhs.Size());
	mSize = rhs.mSize;
	copy_n(rhs.Begin(), (size_t)rhs.Size(), pArray);

	return *this;
}


template<class T>
Array<T>::~Array()
{
	ClearMem();
}

template<class T>
void Array<T>::EnsureCapacity(int newSize)
{
	assert(newSize >= 0 && mSize >= 0 && mCapacity >=0 );
	assert(newSize >= mSize);
	
	int numCtor = newSize - mSize;
	
	if(newSize > mCapacity || !pArray)
	{

		mCapacity = newSize;
		mCapacity = (mCapacity + (mGranularity - 1)) & (-mGranularity);
		
		if(!pArray)
		{
			assert(mSize == 0);
			pArray = (T*) allocator->Alloc(sizeof(T) * (size_t) mCapacity ) ;//new T[mCapacity];
		}
		else
		{
			T* growArray = (T*) allocator->Realloc(pArray, sizeof(T) * (size_t) mCapacity);

			if(growArray != pArray)
			{
				Construct(growArray, (size_t) mSize);
				copy_n(pArray, (size_t) mSize, growArray);
				//delete[] pArray; //fixme, this is wrong, should destruct to size, then free memory
				Destruct(pArray, (size_t) mSize);
				
				allocator->Free(pArray);
				pArray = growArray;
			}
		}

	}
	Construct(pArray + mSize, (size_t) numCtor);
}

template <class T>
void Array<T>::PushBack(const T &val)
{
	int newSize = mSize + 1;
	EnsureCapacity(newSize);
	pArray[mSize] = val;
	mSize = newSize;
}

template <class T>
int Array<T>::Append(const T& val)
{
	int index = mSize;
	Append(val);
	return index;
}

template <class T>
void Array<T>::PushBack()
{
	int newSize = mSize + 1;
	EnsureCapacity(newSize);
	mSize = newSize;
}

template <class T>
void Array<T>::PopBack()
{
	assert(mSize > 0);

	Destruct(&pArray[mSize - 1]);
	--mSize;
}

template <class T>
void Array<T>::Erase(T* pAt)
{
	assert(pAt >= pArray && pAt < pArray + Size() );
	size_t numToCopy = (size_t)( pArray + Size() - 1 - pAt ) / sizeof(T);
	if(numToCopy != 0)
	{
		T* pCopy = pAt + 1;
		copy_n(pCopy, (size_t) numToCopy, pAt);
	}
	
	PopBack();
}

template <class T>
void Array<T>::Erase(int index)
{
	Erase(pArray + index);
}

template <class T>
void Array<T>::EraseSwapBack(T* pAt)
{
	assert(mSize > 0);
	*pAt = *(pArray + mSize - 1);
	PopBack();
}

template <class T>
void Array<T>::Clear()
{
	Destruct(pArray, (size_t)mSize);
	mSize = 0;
}

template <class T>
void Array<T>::ClearMem()
{
	Destruct(pArray, (size_t) mSize);
	allocator->Free(pArray);
	pArray = 0;
	mSize = 0;
	mCapacity = 0;
}

template <class T>
void Array<T>::Reserve(int capacity)
{
	EnsureCapacity(capacity);
}

/*
template <class T>
 void Array<T>::resize(int capacity)
{
	if(mCapacity == capacity)
		return;
	clear_mem();

	
}

template <class T>
 void Array<T>::condense()
{
	resize(size());
}
*/
template <class T>
int Array<T>::Size() const
{
	return mSize;
}

template <class T>
int Array<T>::Capacity() const
{
	return mCapacity;
}

template <class T>
Array<T>::operator const T *() const
{
	return pArray;
}

template<class T>
Array<T>::operator T*()
{
	return pArray;
}
	
template<class T>
const T& Array<T>::operator[] (int index) const
{
	assert(index < mSize);
	return pArray[index];
}

template<class T>
T& Array<T>::operator[] (int index)
{
	assert(index < mSize);
	return pArray[index];
}

template<class T>
T* Array<T>::Begin()
{
	return pArray;
}

template<class T>
const T* Array<T>::Begin() const
{
	return pArray;
}

template <class T>
T* Array<T>::End()
{
	return pArray + mSize;
}

template <class T>
const T* Array<T>::End() const
{
	return pArray + mSize;
}

template <class T>
T* Array<T>::Rbegin()
{
	assert(mSize > 0);
	return pArray + mSize - 1;
}

template <class T>
const T* Array<T>::Rbegin() const
{
	assert(mSize > 0);
	return pArray + mSize - 1;
}

template <class T>
void Array<T>::SetGranularity(int granularity)
{
	assert(IsPowerOfTwo(granularity));
	mGranularity = granularity;
}

template<class T>
const T* Array<T>::Find(const T& val) const
{
	T* v = Find(val);

	return v;
}

template<class T>
T* Array<T>::Find(const T& val)
{
	for(int i = 0; i < mSize; i++)
	{
		if(pArray[i] == val )
			return pArray + i;
	}

	return NULL;
}

template<class T>
int Array<T>::FindIndex(const T& val) const
{
	for(int i = 0; i < mSize; i++)
	{
		if(pArray[i] == val)
			return i;
	}
	return -1;
}


}

#endif