#ifndef YU_DEQUEUE_H
#define YU_DEQUEUE_H

#include "../core/thread.h"
#include "../core/bit.h"
#include <assert.h>
#include <atomic>

namespace yu
{

template<class T, int numElem>
class LockSpscFifo
{
public:
	LockSpscFifo();
	void			Enq(T& elem);
	T&				Deq();
	bool			IsFull();
	bool			IsNull();

private:
	unsigned int	readPos;
	T				elems[numElem];
	unsigned int	writePos;
	Mutex			mutex;
};

template<class T, int numElem> 
LockSpscFifo<T, numElem>::LockSpscFifo() : readPos(0), writePos(0)
{

}

template<class T, int numElem>
void LockSpscFifo<T, numElem>::Enq(T& elem)
{
	ScopedLock lock(mutex);
	assert(IsPowerOfTwo(numElem));
	elems[writePos & (numElem - 1)] = elem;
	writePos++;
}

template<class T, int numElem>
T& LockSpscFifo<T, numElem>::Deq()
{
	ScopedLock lock(mutex);
	assert(IsPowerOfTwo(numElem));
	T& elemToGet = elems[readPos & (numElem - 1)];
	readPos++;
	return elemToGet;
}

template<class T, int numElem>
bool LockSpscFifo<T, numElem>::IsNull()
{
	ScopedLock lock(mutex);
	return (writePos == readPos);
}

template<class T, int numElem>
bool LockSpscFifo<T, numElem>::IsFull()
{
	ScopedLock lock(mutex);
	return ((writePos - readPos) == numElem);
}

template<class T, int numElem>
class LocklessSpscFifo
{
public:

private:
	std::atomic<unsigned int>	readPos;
	T							elems[numElem];
	std::atomic<unsigned int>	writePos;
};

}

#endif