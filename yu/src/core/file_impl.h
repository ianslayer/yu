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
	dataBlob.data = new char[fileLen];
	dataBlob.dataLen = ReadFile(path, dataBlob.data, fileLen);

	return dataBlob;
}
	
void FreeDataBlob(DataBlob blob)
{
	delete[] blob.data;
}

char* BuildDataPath(const  char* relPath, char* buffer,size_t bufferLen)
{
	const char* dataPath = DataPath();
	StringBuilder pathBuilder(buffer, bufferLen);
	pathBuilder.Cat(dataPath);
	pathBuilder.Cat(relPath);
	return buffer;
}


}
