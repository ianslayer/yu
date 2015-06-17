#ifndef YU_STRING_H
#define YU_STRING_H

namespace yu
{

//interface to avoid include explosion
void *memcpy(void *dest, const void *src, size_t n);
void memset(void* dst, int val, size_t size);

int strcmp ( const char * str1, const char * str2 );
size_t strlen ( const char * str );
char * strncpy(char * destination, const char * source, size_t num);
	
#define SID(s) InternStr(s)
#define CSID(s) //TODO: should be transformed to #define CSID(s, id) id

#define TSID(s, t) InternStr(s, t)
#define TCSID(s, t) //TODO: should be transformed to #define TCSID(s, id, t) id

typedef u32 StringId;
struct StringRef
{
	StringId		id;
	const char*		str;
};

struct StrTable;
class ArenaAllocator;
StrTable*		NewStrTable(bool threadSafe, bool stripString);
StringRef		InternStr(const char*, StrTable* table);

void			InitSysStrTable();
void			FreeSysStrTable();
StringRef		InternStr(const char*);
StringRef		CompiledStr(StringId id, const char* str);

}

#endif
