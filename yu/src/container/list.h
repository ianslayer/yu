#ifndef YU_LIST_H
#define YU_LIST_H
#include <atomic>

namespace yu
{

struct ListNode
{
	atomic<ListNode*> next;
};
	
}

#endif
