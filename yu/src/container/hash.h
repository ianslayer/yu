#ifndef YU_HASH_H
#define YU_HASH_H

#include "../core/new.h"
#include "../core/type.h"
#include "../core/allocator.h"

namespace yu
{

template <class Key, class T, typename HashFunc = typename Key::HashFunc>
class HashTable
{
public:
	HashTable(int tableSize = 256, Allocator* allocator = gDefaultAllocator);
	~HashTable();

	void Set(const Key& key, const T& value);
	bool Get(const Key& key, T** value = nullptr) const;
	bool Remove(const Key& key);

	void Clear();

private:

	struct HashNode
	{
		HashNode(const Key& _key, const T& _value)
			: key(_key), value(_value), next(nullptr)
		{

		}

		Key       key;
		T         value;
		HashNode* next;
	};

	HashNode**	heads;
	Allocator*	allocator;
	int			tableSize;
	int			numEntries;
	int			tableSizeMask;
};

template <class Key, class T, class HashFunc>
HashTable<Key, T, HashFunc>::HashTable(int _tableSize /* = 256 */, Allocator* _allocator) : allocator(_allocator)
{
	assert(IsPowerOfTwo(_tableSize));

	tableSize = _tableSize;
	heads = (HashNode**) allocator->Alloc(sizeof(HashNode*) * tableSize) ;//HashNode*[tableSize];
	memset(heads, 0, sizeof(*heads) * tableSize );
	numEntries = 0;
	tableSizeMask = tableSize - 1;
}

template <class Key, class T, class HashFunc>
HashTable<Key, T, HashFunc>::~HashTable()
{
	Clear();
	allocator->Free(heads);//delete [] heads;
}

template <class Key, class T, class HashFunc>
void HashTable<Key, T, HashFunc>::Set(const Key& key, const T& value)
{
	HashFunc Hash;
	int hash = Hash(key) & tableSizeMask;
	HashNode* node, **nextPtr;
	for(nextPtr = &(heads[hash]), node = *nextPtr; node!= nullptr; nextPtr = &node->next, node = *nextPtr)
	{
		if(node->key == key)
		{
			node->value = value;
			return;
		}
		//if(node->key > key)
		//	break;
	}

	numEntries++;
	void* nodeMem = allocator->Alloc(sizeof(HashNode));
	*nextPtr = new (nodeMem) HashNode(key, value);
	(*nextPtr)->next = node;
}

template <class Key, class T, class HashFunc>
bool HashTable<Key, T, HashFunc>::Get(const Key &key, T **value) const
{
	HashFunc Hash;
	int hash = Hash(key) & tableSizeMask;
	HashNode* node;
	for(node = heads[hash]; node != nullptr; node = node->next)
	{
		if(node->key == key)
		{
			if(value)
				*value = &node->value;
			return true;
		}
		//if(node->key > key)
		//{
		//	break;
		//}
	}

	if(value)
		*value = NULL;
	return false;
}

template<class Key, class T, class HashFunc>
bool HashTable<Key, T, HashFunc>::Remove(const Key& key)
{
	HashFunc Hash;
	int hash = Hash(key) & tableSizeMask;
	HashNode* node, *prev;
	HashNode** head = &heads[hash];
	for(prev = nullptr, node = heads[hash]; node != nullptr; prev = node, node = node->next)
	{
		if(node->key == key)
		{
			if ( prev ) 
			{
				prev->next = node->next;
			} 
			else 
			{
				*head = node->next;
			}

			numEntries--;
			//delete node;
			node->~HashNode();
			allocator->Free(node);
			return true;
		}
		if(node->key > key)
		{
			break;
		}
	}

	return false;
}

template <class Key, class T, class HashFunc>
void HashTable<Key, T, HashFunc>::Clear()
{
	for(int i = 0; i < tableSize; i++)
	{
		HashNode* node = heads[i];
		while(node != nullptr)
		{
			HashNode* deleteNode = node;
			node = node->next;
			//delete deleteNode;
			deleteNode->~HashNode();
			allocator->Free(deleteNode);
		}
		heads[i] = nullptr;
	}

	numEntries = 0;
}

}

#endif