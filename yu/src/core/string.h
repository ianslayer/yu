#ifndef YU_STRING_H
#define YU_STRING_H

namespace yu
{


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
StrTable*		NewStrTable(bool threadSafe, bool stripString, ArenaAllocator* strBufferAllocator, ArenaAllocator* nodeAllocator);
StringRef		InternStr(const char*, StrTable* table);

void			InitSysStrTable(class Allocator* allocator);
void			FreeSysStrTable();
StringRef		InternStr(const char*);
StringRef		CompiledStr(StringId id, const char* str);

}

#endif
