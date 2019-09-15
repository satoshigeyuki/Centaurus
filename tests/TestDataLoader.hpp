#pragma once

#include <iostream>
#include <fstream>

#include <Windows.h>

char *LoadTextAligned(const char *filename)
{
    HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_HANDLE_VALUE)
        return NULL;

    DWORD dwFileSize = GetFileSize(hFile, NULL);
    size_t alloc_size = (dwFileSize + 32) / 32 * 32;

    char *buf = (char *)_aligned_malloc(alloc_size, 32);
    
    DWORD dwBytesRead;
    ReadFile(hFile, buf, dwFileSize, &dwBytesRead, NULL);

    buf[dwBytesRead] = 0;

    return buf;
}