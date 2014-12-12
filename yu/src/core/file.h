#ifndef YU_FILE_H
#define YU_FILE_H

namespace yu
{
size_t FileSize(const char* path);
size_t ReadFile(const char* path, void* buffer, size_t bufferLen);
}

#endif
