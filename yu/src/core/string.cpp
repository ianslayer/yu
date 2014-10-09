#include <string.h>
namespace yu
{

void *memcpy(void *dest, const void *src, size_t n)
{
	return ::memcpy(dest, src, n);
}


}
