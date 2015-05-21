#if !defined YU_LIB_H

#include "platform.h"
#if defined YU_OS_WIN32
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <windows.h>
#elif defined YU_OS_MAC
	#include <mach/mach.h>
	#include <pthread.h>
	#include <semaphore.h>
#endif
#include <atomic>
#include <assert.h>

namespace yu
{

//memory manipulation

//interface to avoid include explosion
void *memcpy(void *dest, const void *src, size_t n);
void memset(void* dst, int val, size_t size);

int strcmp ( const char * str1, const char * str2 );
size_t strlen ( const char * str );
char * strncpy(char * destination, const char * source, size_t num);

char ToLowerCase(char c);
char ToUpperCase(char c);

class StringBuilder
{
public:
	StringBuilder(char* buf, size_t bufSize);
	StringBuilder(char* str, size_t bufSize, size_t strLen);
	bool	Cat(const char* source, size_t len);
	bool	Cat(const char* source);
	bool	Cat(int i);
	bool	Cat(unsigned int i);
	bool	Cat(bool f);
	char*	strBuf;
	const size_t bufSize;
	size_t	strLen;
};	
	
//platform thread primitive
struct Mutex
{
	Mutex();
	~Mutex();

	void Lock();
	void Unlock();
#if defined YU_OS_WIN32
	CRITICAL_SECTION m;
#else
	pthread_mutex_t m;
#endif
};

struct RWMutex
{
	RWMutex();
	~RWMutex();

	void ReaderLock();
};

struct ScopedLock
{
	explicit ScopedLock(Mutex& m);
	~ScopedLock();

	Mutex& m;
};

struct Locker
{
	explicit Locker(Mutex& m);
	~Locker();

	void Lock();

	bool locked;
	Mutex& m;
};	


struct CondVar
{
	CondVar();
	~CondVar();

#if defined YU_OS_WIN32
	CONDITION_VARIABLE cv;
#else
	pthread_cond_t cv;
#endif
};
void			WaitForCondVar(CondVar& cv, Mutex& m);
void			NotifyCondVar(CondVar& cv);
void			NotifyAllCondVar(CondVar& cv);

struct Event
{
	Event() : signaled(false)
	{}
	CondVar	cv;
	Mutex	cs;
	std::atomic<bool>	signaled;
};
void			WaitForEvent(Event& ev);
void			SignalEvent(Event& ev);
void			ResetEvent(Event& ev);

struct Semaphore
{
	Semaphore(int initCount, int maxCount);
	~Semaphore();

#if defined YU_OS_WIN32
	HANDLE sem;
#elif defined YU_OS_MAC
	sem_t* sem;
#else
#	error semaphore not implemented
#endif
};
void			WaitForSem(Semaphore& sem);
void			SignalSem(Semaphore& sem);



}

#define YU_LIB_H
#endif


#if defined YU_LIB_IMPL

#if !defined YU_LIB_IMPLED
#include <string.h>
namespace yu
{

void *memcpy(void *dest, const void *src, size_t n)
{
	return ::memcpy(dest, src, n);
}

void memset(void* dst, int val, size_t size)
{
	::memset(dst, val, size);
}

int strcmp ( const char * str1, const char * str2 )
{
	return ::strcmp(str1, str2);
}

size_t strlen ( const char * str )
{
	return ::strlen(str);
}

char * strncpy ( char * destination, const char * source, size_t num  )
{
#if defined YU_OS_WIN32
	errno_t err = ::strncpy_s(destination, num+1, source, num);
	 return destination;
#else
	return ::strncpy(destination, source, num);
#endif
}

char ToLowerCase(char c)
{
	if( c <= 'Z' && c >= 'A' )
		return c - 'A' + 'a';
	return c;
}

char ToUpperCase(char c)
{
	if( c <= 'z' && c >= 'a' )
		return c - 'a' + 'A';
	return c;
}

StringBuilder::StringBuilder(char* buf, size_t _bufSize)
	: strBuf(buf), bufSize(_bufSize), strLen(0)
{

}

StringBuilder::StringBuilder(char* buf, size_t _bufSize, size_t _strLen)
	: strBuf(buf), bufSize(_bufSize), strLen(_strLen)
{
	assert(bufSize > strLen);
}


bool StringBuilder::Cat(const char* source, size_t len)
{
	if (strLen + 1 + len > bufSize)
		return false;

	char* dest = strBuf + strLen;
	size_t catSize = 0;
	for (catSize = 0; catSize < len && *source; catSize++)
	{
		*dest++ = *source++;
	}
	*dest = 0;

	strLen += catSize;

	return true;
}

bool StringBuilder::Cat(const char* source)
{
	return Cat(source, strlen(source));
}

bool StringBuilder::Cat(unsigned int i)
{
	size_t convertLen = 0;
	unsigned int li = i;
	do{
		li /= 10;
		convertLen++;
	}
	while(li != 0);

	
	if(strLen + 1 + convertLen > bufSize)
		return false;
	
	char* dest = strBuf + strLen + convertLen;
	*dest = 0;
	li = i;
	for(size_t catSize = 0; catSize < convertLen; catSize++)
	{
		*--dest = ((li % 10) + '0');
		li /= 10;
	}


	strLen += convertLen;
	return true;
}

	
ScopedLock::ScopedLock(Mutex& _m) : m(_m)
{
	m.Lock();
}

ScopedLock::~ScopedLock()
{
	m.Unlock();
}

Locker::Locker(Mutex& _m) : locked(false), m(_m)
{
}

Locker::~Locker()
{
	if (locked)
		m.Unlock();
}

void Locker::Lock()
{
	m.Lock();
	locked = true;
}

void WaitForEvent(Event& ev)
{

	if (ev.signaled == true)
	{
		return;
	}

	ScopedLock lock(ev.cs);
	if (ev.signaled == true)
	{
		return;
	}
	WaitForCondVar(ev.cv, ev.cs);

}

void SignalEvent(Event& ev)
{
	ScopedLock lock(ev.cs);
	ev.signaled = true;
	NotifyAllCondVar(ev.cv);
}

void ResetEvent(Event& ev)
{
	ScopedLock lock(ev.cs);
	ev.signaled = false;
}

#if defined YU_OS_MAC

Mutex::Mutex()
{
	pthread_mutex_init(&m, nullptr);
}

Mutex::~Mutex()
{
	pthread_mutex_destroy(&m);
}

void Mutex::Lock()
{
	pthread_mutex_lock(&m);
}

void Mutex::Unlock()
{
	pthread_mutex_unlock(&m);
}

CondVar::CondVar()
{
	pthread_cond_init(&cv, nullptr);
}

CondVar::~CondVar()
{
	pthread_cond_destroy(&cv);
}

void WaitForCondVar(CondVar& cv, Mutex& cs)
{
	pthread_cond_wait(&cv.cv, &cs.m);
}

void NotifyCondVar(CondVar& cv)
{
	pthread_cond_signal(&cv.cv);
}

void NotifyAllCondVar(CondVar& cv)
{
	pthread_cond_broadcast(&cv.cv);
}

YU_GLOBAL char yuSemName[128];
YU_GLOBAL std::atomic<unsigned int> semName;
Semaphore::Semaphore(int initCount, int maxCount)
{
	unsigned int name = semName.fetch_add(1);
	StringBuilder nameStr(yuSemName, sizeof(yuSemName));
	nameStr.Cat("yuSem");
	nameStr.Cat(name);
	sem = sem_open(yuSemName, O_CREAT, 0644, (unsigned int)initCount);
}

Semaphore::~Semaphore()
{
	sem_close(sem);
}

void WaitForSem(Semaphore& sem)
{
	sem_wait(sem.sem);
}

void SignalSem(Semaphore& sem)
{
	sem_post(sem.sem);
}

}

#define YU_LIB_IMPLED
#endif
#endif

#endif
