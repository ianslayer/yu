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
	bool			Enqueue(const T& elem);
	bool			Dequeue(T& data);

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
bool LockSpscFifo<T, numElem>::Enqueue(const T& elem)
{
	ScopedLock lock(mutex);
	assert(IsPowerOfTwo(numElem));
	if ((writePos - readPos) == numElem)
		return false;

	elems[writePos & (numElem - 1)] = elem;
	writePos++;

	return true;
}

template<class T, int numElem>
bool LockSpscFifo<T, numElem>::Dequeue(T& data)
{
	ScopedLock lock(mutex);
	assert(IsPowerOfTwo(numElem));
	if (writePos == readPos)
		return false;

	data = elems[readPos & (numElem - 1)];
	readPos++;
	return true;
}

template<class T, int numElem>
class LocklessSpscFifo
{
public:
	LocklessSpscFifo();
	bool			Enqueue(const T& elem);
	bool			Dequeue(T& data);
private:
	std::atomic<unsigned int>	readPos;
	T							elems[numElem];
	std::atomic<unsigned int>	writePos;
};

template<class T, int numElem>
LocklessSpscFifo<T, numElem>::LocklessSpscFifo() : readPos(0), writePos(0)
{

}

template<class T, int numElem>
bool LocklessSpscFifo<T, numElem>::Enqueue(const T& elem)
{
	assert(IsPowerOfTwo(numElem));

	unsigned int localWritePos = writePos.load(std::memory_order_acquire); //this maybe relaxed?
	unsigned int localReadPos = readPos.load(std::memory_order_acquire);

	if ((localWritePos - localReadPos) == numElem)
		return false;

	elems[localWritePos & (numElem - 1)] = elem;
	writePos.fetch_add(1, std::memory_order_release);

	return true;
}

template<class T, int numElem>
bool LocklessSpscFifo<T, numElem>::Dequeue(T& data)
{
	assert(IsPowerOfTwo(numElem));

	unsigned int localReadPos = readPos.load(std::memory_order_acquire); //this maybe relaxed?
	unsigned int localWritePos = writePos.load(std::memory_order_acquire); 

	if (localWritePos == localReadPos)
		return false;

	data = elems[localReadPos & (numElem - 1)];
	readPos.fetch_add(1, std::memory_order_release);
	return true;
}

#define LOCKLESS_SPSC_FIFO

#if defined LOCKLESS_SPSC_FIFO
	template<class T, int num> using SpscFifo = LocklessSpscFifo < T, num > ;  //c++ 11 black magic...
#else
	template<class T, int num> using SpscFifo = LockSpscFifo < T, num > ;
#endif

//from 1024 cores http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
template<typename T, int numElem>
class MpmcFifo
{
public:
	MpmcFifo()
		:buffer_mask_(numElem - 1)
	{
		assert((numElem >= 2) &&
			((numElem & (numElem - 1)) == 0));
		for (size_t i = 0; i != numElem; i += 1)
			buffer_[i].sequence_.store(i, std::memory_order_relaxed);
		enqueue_pos_.store(0, std::memory_order_relaxed);
		dequeue_pos_.store(0, std::memory_order_relaxed);
	}

	~MpmcFifo()
	{
	}

	bool Enqueue(T const& data)
	{
		cell_t* cell;
		size_t pos = enqueue_pos_.load(std::memory_order_relaxed);
		for (;;)
		{
			cell = &buffer_[pos & buffer_mask_];
			size_t seq =
				cell->sequence_.load(std::memory_order_acquire);
			intptr_t dif = (intptr_t)seq - (intptr_t)pos;
			if (dif == 0)
			{
				if (enqueue_pos_.compare_exchange_weak
					(pos, pos + 1, std::memory_order_relaxed))
					break;
			}
			else if (dif < 0)
				return false;
			else
				pos = enqueue_pos_.load(std::memory_order_relaxed);
		}
		cell->data_ = data;
		cell->sequence_.store(pos + 1, std::memory_order_release);
		return true;
	}

	bool Dequeue(T& data)
	{
		cell_t* cell;
		size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
		for (;;)
		{
			cell = &buffer_[pos & buffer_mask_];
			size_t seq =
				cell->sequence_.load(std::memory_order_acquire);
			intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);
			if (dif == 0)
			{
				if (dequeue_pos_.compare_exchange_weak
					(pos, pos + 1, std::memory_order_relaxed))
					break;
			}
			else if (dif < 0)
				return false;
			else
				pos = dequeue_pos_.load(std::memory_order_relaxed);
		}
		data = cell->data_;
		cell->sequence_.store
			(pos + buffer_mask_ + 1, std::memory_order_release);
		return true;
	}

private:
	struct cell_t
	{
		std::atomic<size_t>   sequence_;
		T                     data_;
	};

	static size_t const     cacheline_size = 64;
	typedef char            cacheline_pad_t[cacheline_size];

	cacheline_pad_t         pad0_;
	cell_t					buffer_[numElem];
	size_t const            buffer_mask_;
	cacheline_pad_t         pad1_;
	std::atomic<size_t>     enqueue_pos_;
	cacheline_pad_t         pad2_;
	std::atomic<size_t>     dequeue_pos_;
	cacheline_pad_t         pad3_;

	MpmcFifo(MpmcFifo const&);
	void operator = (MpmcFifo const&);
};

}

#endif