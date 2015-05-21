#include "crc32.h"
#include "string.h"
#include <string.h>
#include <assert.h>
#include <new>
namespace yu
{


struct StringIdHashFunc
{
	u32 operator() (u32 id)
	{
		return id;
	}
};

#define CHECK_DUPLICATE

const YU_GLOBAL int HASH_TABLE_SIZE = 1024;
const YU_GLOBAL int  HASH_MASK = HASH_TABLE_SIZE - 1;
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

void InitSysStrTable(Allocator* allocator)
{
	gStrArenaAllocator = CreateArena(64 * 1024, allocator);
	gStrNodeAllocator = CreateArena(64 * 1024, allocator);
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
