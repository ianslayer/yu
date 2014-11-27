#ifndef YU_STRING_H
#define YU_STRING_H

namespace yu
{

//interface to avoid include explosion
void *memcpy(void *dest, const void *src, size_t n);
void memset(void* dst, int val, size_t size);

int strcmp ( const char * str1, const char * str2 );
size_t strlen ( const char * str );
char * strncpy ( char * destination, const char * source, size_t num );

#define SID(s) s

class String
{

};

class StringRef
{

};



}

#endif