#pragma once

#if defined(CENTAURUS_BUILD_WINDOWS)
#include <Windows.h>
#elif defined(CENTAURUS_BUILD_LINUX)
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

namespace Centaurus
{
class Input
{
protected:
    const void *m_buffer;
    size_t m_length;
public:
	Input()
		: m_buffer(NULL), m_length(0)
	{
	}
    Input(const void *input, size_t length)
        : m_buffer(input), m_length(length)
    {
    }
    virtual ~Input()
    {
    }
    const void *get_buffer() const
    {
        return m_buffer;
    }
};
class MemoryInput : public Input
{
public:
    MemoryInput(const char *input, size_t length)
        : Input(input, length)
    {
    }
    virtual ~MemoryInput()
    {
    }
};
class MappedFileInput : public Input
{
#if defined(CENTAURUS_BUILD_WINDOWS)
    HANDLE hFile, hMapping;
#elif defined(CENTAURUS_BUILD_LINUX)
    int fd;
#endif
public:
    MappedFileInput(const char *filename)
    {
#if defined(CENTAURUS_BUILD_WINDOWS)
        hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

        DWORD dwFileSizeHigh;
        DWORD dwFileSizeLow = GetFileSize(hFile, &dwFileSizeHigh);

        hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, dwFileSizeHigh, dwFileSizeLow, NULL);

        m_length = ((size_t)dwFileSizeHigh << 32) | dwFileSizeLow;

        m_buffer = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
#elif defined(CENTAURUS_BUILD_LINUX)
        fd = open(filename, O_RDONLY);

        struct stat sb;

        fstat(fd, &sb);

        m_length = sb.st_size;

        m_buffer = mmap(NULL, m_length, PROT_READ, MAP_PRIVATE, fd, 0);
#endif
    }
    virtual ~MappedFileInput()
    {
#if defined(CENTAURUS_BUILD_WINDOWS)
        if (m_buffer != NULL)
            UnmapViewOfFile(m_buffer);
        if (hMapping != NULL)
            CloseHandle(hMapping);
        if (hFile != NULL)
            CloseHandle(hFile);
#elif defined(CENTAURUS_BUILD_LINUX)
        if (m_buffer != NULL)
            munmap(const_cast<void *>(m_buffer), m_length);
        if (fd != -1)
            close(fd);
#endif
    }
};
}
