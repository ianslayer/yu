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
	StrTable(bool _threadSafe, bool _stripString)
		: threadSafe(_threadSafe), stripString(_stripString)
	{
		memset(head, 0, sizeof(head));
		strBuffer = new ArenaAllocator(4 * 1024 * 1024, GetCurrentAllocator());
		nodeAllocator = GetCurrentAllocator();
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
	Allocator*		nodeAllocator;

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
	PushAllocator(table->nodeAllocator);	
	StrTable::Node* newNode = new StrTable::Node;
	PopAllocator();
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

StrTable* NewStrTable(bool threadSafe, bool stripString)
{
	StrTable* strTable = new StrTable(threadSafe, stripString);
	return strTable;
}

void FreeStrTable(StrTable* table)
{

}

StrTable* gStrTable;

void InitSysStrTable()
{
	gStrTable = NewStrTable(true, false);
}

void FreeSysStrTable()
{

}

StringRef InternStr(const char* str)
{
	return InternStr(str, gStrTable);
}

}
