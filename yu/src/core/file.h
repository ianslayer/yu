#ifndef YU_FILE_H
#define YU_FILE_H
#include "string.h"
namespace yu
{
const char* WorkingDir();
const char* ExePath();
const char* DataPath();
size_t GetDirFromPath(const char* path, char* outDirPath, size_t bufLength);
	
size_t FileSize(const char* path);
size_t ReadFile(const char* path, void* buffer, size_t bufferLen);
size_t SaveFileOverWrite(const char* path, void* buffer, size_t bufferLen);

struct			DataBlob
{
	StringRef	sourcePath;
	void*		data;
	size_t		dataLen;
};

DataBlob ReadDataBlob(const char* path);
void FreeDataBlob(DataBlob blob);

char* BuildDataPath(const  char* relPath, char* buffer,size_t bufferLen);

}
#endif
