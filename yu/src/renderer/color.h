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
}
#endif