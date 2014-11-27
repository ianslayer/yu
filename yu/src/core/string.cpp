#include <string.h>

namespace yu
{

void *memcpy(void *dest, const void *src, size_t n)
{
	return ::memcpy(dest, src, n);
}

void memset(void* dst, int val, size_t size)
{
	::memset(dst, val, size);
}

int strcmp ( const char * str1, const char * str2 )
{
	return ::strcmp(str1, str2);
}

size_t strlen ( const char * str )
{
	return ::strlen(str);
}

char * strncpy ( char * destination, const char * source, size_t num  )
{
	return ::strncpy(destination, source, num);
}


}
