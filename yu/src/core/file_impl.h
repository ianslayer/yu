#include "file.h"
namespace yu
{

DataBlob ReadDataBlob(const char* path)
{
	DataBlob dataBlob = {};

	dataBlob.sourcePath = InternStr(path);
	size_t fileLen = FileSize(path);
	if (fileLen == 0)
	{
		Log("warning, ReadDataBlob: file: %s is empty\n ");
		return dataBlob;
	}
	dataBlob.data = gDefaultAllocator->Alloc(fileLen);
	dataBlob.dataLen = ReadFile(path, dataBlob.data, fileLen);

	return dataBlob;
}
void FreeDataBlob(DataBlob blob)
{
	gDefaultAllocator->Free(blob.data);
}

}