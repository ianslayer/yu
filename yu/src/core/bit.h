#ifndef YU_BIT_H
#define YU_BIT_H

namespace yu
{
	
bool IsPowerOfTwo(int n)
{
	return n > 0 && ( n & (n-1) ) == 0;
}

bool IsPowerOfTwo(unsigned int n)
{
	return ( n & (n-1) ) == 0;
}
	
}

#endif
