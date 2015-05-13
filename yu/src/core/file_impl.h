#include "file.h"
namespace yu
{

DataBlob ReadDataBlob(const char* path, Allocator* allocator)
{
	DataBlob dataBlob = {};

	dataBlob.sourcePath = InternStr(path);
	size_t fileLen = FileSize(path);
	if (fileLen == 0)
	{
		Log("warning, ReadDataBlob: file: %s is empty\n ");
		return dataBlob;
	}
	dataBlob.data = allocator->Alloc(fileLen);
	dataBlob.dataLen = ReadFile(path, dataBlob.data, fileLen);

	return dataBlob;
}
	
void FreeDataBlob(DataBlob blob, Allocator* allocator)
{
	allocator->Free(blob.data);
}

}
