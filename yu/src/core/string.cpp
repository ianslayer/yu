#include "platform.h"
#include "allocator.h"
#include "thread.h"
#include "crc32.h"
#include "string.h"
#include "log.h"
#include <string.h>
#include <new>
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

struct StringIdHashFunc
{
	u32 operator() (u32 id)
	{
		return id;
	}
};

#define CHECK_DUPLICATE

const static int HASH_TABLE_SIZE = 1024;
const static int  HASH_MASK = HASH_TABLE_SIZE - 1;
struct StrTable
{
	StrTable(bool _threadSafe, bool _stripString, ArenaAllocator* _strBufferAllocator, ArenaAllocator* _nodeAllocator)
		: threadSafe(_threadSafe), stripString(_stripString), strBuffer(_strBufferAllocator), nodeAllocator(_nodeAllocator)
	{
		memset(head, 0, sizeof(head));
	}
	~StrTable()
	{
	}

	struct Node
	{
		StringId		id;
		const char*		str;
		Node*			next;
	};

	Node*			head[HASH_TABLE_SIZE]; //TODO: FALSE SHARING
	mutable Mutex	m[HASH_TABLE_SIZE];
	const bool		threadSafe;
	const bool		stripString;
	ArenaAllocator* strBuffer;
	ArenaAllocator* nodeAllocator;

};

StringRef InternStr(const char* str, StrTable* table)
{
	size_t strLen = strlen(str);
	unsigned int hash = CRC32(str, strLen);

	if (table->stripString)
	{
		return StringRef{hash, nullptr};
	}

	Locker locker(table->m[hash & HASH_MASK]);
	if (table->threadSafe)
	{
		locker.Lock();
	}

#if defined CHECK_DUPLICATE
	for (StrTable::Node* node = table->head[hash & HASH_MASK]; node; node = node->next)
	{
		if (node->id == hash)
		{
			int test = strcmp(str, node->str);

			if (test != 0)
			{
				Log("error, string: \"%s\" id duplicated with: %s\n", str, node->str);
			}
		}
	}
#endif

	for (StrTable::Node* node = table->head[hash & HASH_MASK]; node; node = node->next)
	{
		if (node->id == hash)
		{
			StringRef ref = {hash, node->str};
			return ref;
		}
	}
	StrTable::Node* newNode = (StrTable::Node*) table->nodeAllocator->Alloc(sizeof(StrTable::Node));
	char* strBuffer = (char*) table->strBuffer->Alloc(strLen + 1);
	strncpy(strBuffer, str, strLen);

	newNode->id = hash;
	newNode->str = strBuffer;

	newNode->next = table->head[hash & HASH_MASK];
	table->head[hash & HASH_MASK] = newNode;

	StringRef ref = {hash, strBuffer};

	return ref;
}

const char* GetString(StringId id, const StrTable* table)
{
	Locker locker(table->m[id & HASH_MASK]);
	if (table->threadSafe)
	{
		locker.Lock();
	}

	for (StrTable::Node* node = table->head[id & HASH_MASK]; node; node = node->next)
	{
		if (node->id == id)
		{
			return node->str;
		}
	}
	return nullptr;
}

StrTable* NewStrTable(bool threadSafe, bool stripString, ArenaAllocator* strBufferAllocator, ArenaAllocator* nodeAllocator)
{
	StrTable* strTable = new(nodeAllocator->Alloc(sizeof(*strTable))) StrTable(threadSafe, stripString, strBufferAllocator, nodeAllocator);
	return strTable;
}

void FreeStrTable(StrTable* table)
{

}

StrTable* gStrTable;
ArenaAllocator* gStrArenaAllocator;
ArenaAllocator* gStrNodeAllocator;

void InitSysStrTable()
{
	gStrArenaAllocator = new ArenaAllocator();
	gStrNodeAllocator = new ArenaAllocator();
	gStrTable = NewStrTable(true, false, gStrArenaAllocator, gStrNodeAllocator);
}

void FreeSysStrTable()
{
	delete gStrNodeAllocator;
	delete gStrArenaAllocator;
}

StringRef InternStr(const char* str)
{
	return InternStr(str, gStrTable);
}

}