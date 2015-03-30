#ifndef YU_FILE_H
#define YU_FILE_H
#include "string.h"
namespace yu
{
const char* WorkingDir();
size_t FileSize(const char* path);
size_t ReadFile(const char* path, void* buffer, size_t bufferLen);

struct			DataBlob
{
	StringRef	sourcePath;
	void*		data;
	size_t		dataLen;
};

inline DataBlob ReadDataBlob(const char* path)
{
	DataBlob dataBlob = {};

	dataBlob.sourcePath = InternStr(path);
	size_t fileLen = FileSize(path);
	if(fileLen == 0)
	{
		Log("warning, ReadDataBlob: file: %s is empty\n ");
		return dataBlob;
	}
	dataBlob.data = gDefaultAllocator->Alloc(fileLen);
	dataBlob.dataLen = ReadFile(path, dataBlob.data, fileLen);
	
	return dataBlob;
}

inline void FreeDataBlob(DataBlob blob)
{
	gDefaultAllocator->Free(blob.data);
}


}
#endif
