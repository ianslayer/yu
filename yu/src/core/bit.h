#ifndef YU_BIT_H
#define YU_BIT_H
#include "platform.h"

namespace yu
{
	
YU_INLINE bool IsPowerOfTwo(int n)
{
	return n > 0 && ( n & (n-1) ) == 0;
}

YU_INLINE bool IsPowerOfTwo(unsigned int n)
{
	return ( n & (n-1) ) == 0;
}
	
#if defined YU_CPU_X86_64
YU_INLINE bool IsPowerOfTwo(size_t n)
{
	return ( n & (n-1) ) == 0;
}
#endif

template<class T>
YU_INLINE  T RoundUp(T x, T n)
{
	return (x + n - 1)& ~(n - 1);
}

}

#endif
