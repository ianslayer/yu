#ifndef YU_MATH_H
#define YU_MATH_H

#include <math.h>
#include "../core/platform.h"

namespace yu
{

template<class T> YU_INLINE T max(T v0, T v1)
{
	return (v0 > v1)?v0:v1;
}

template<class T> YU_INLINE T min(T v0, T v1)
{
	return (v0 < v1)?v0:v1;
}

template<class T> YU_INLINE void swap(T v0, T v1)
{
	T tmp;
	tmp = v0;
	v0 = v1;
	v1 = tmp;
}


}

#endif
