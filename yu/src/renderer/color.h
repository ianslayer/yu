#ifndef YU_COLOR_H
#define YU_COLOR_H

#include "../core/platform.h"

namespace yu
{
struct Color
{
	union
	{
		struct
		{
			u8 r, g, b, a;
		};
		u8 c[4];
	};
};

YU_INLINE Color _Color(u8 r, u8 g, u8 b, u8 a)
{
	Color c;
	c.r = r; c.g = g; c.b = b; c.a = a;
	return c;
}

}
#endif