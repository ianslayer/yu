#include "../math/yu_math.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace yu
{

	size_t FileSize(const char* path)
{
	HANDLE fileHandle = CreateFileA(path, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	LARGE_INTEGER fileSize;
	BOOL getSizeSuccess = GetFileSizeEx(fileHandle, &fileSize);
	CloseHandle(fileHandle);
	if (getSizeSuccess)
	{
		return fileSize.QuadPart;
	}
	return 0;
}


size_t ReadFile(const char* path, void* buffer, size_t bufferLen)
{

	size_t fileSize = FileSize(path);

	HANDLE fileHandle = CreateFileA(path, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	DWORD readSize;
	BOOL readSuccess = ::ReadFile(fileHandle, buffer, (DWORD)min(fileSize, bufferLen), &readSize, nullptr);
	CloseHandle(fileHandle);

	return readSize;
}


}