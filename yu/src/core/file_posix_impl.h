#include "../math/yu_math.h"
#include <stdlib.h>
namespace yu
{

const char* WorkingDir()
{
	YU_LOCAL_PERSIST char cwd[256];
	getcwd(cwd, sizeof(cwd));
	return cwd;
}

size_t FileSize(const char* path)
{
	FILE* fp = fopen(path, "rb");

	if(fp == NULL)
		return 0;

	fseek(fp, 0, SEEK_END);
	size_t seekLen = (size_t) ftell(fp);
	fseek(fp, 0, SEEK_SET);

	fclose(fp);

	return seekLen;
}

size_t ReadFile(const char* path, void* buffer, size_t buffer_len)
{
	FILE* fp = fopen(path, "rb");

	if(fp == NULL)
		return 0;

	fseek(fp, 0, SEEK_END);
	size_t seekLen = (size_t)ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	size_t readLen = fread(buffer, 1, min(seekLen, buffer_len), fp);
	fclose(fp);

	return readLen;
}

}