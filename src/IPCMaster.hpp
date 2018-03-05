#pragma once

#if defined(CENTAURUS_BUILD_WINDOWS)
#include <Windows.h>
#elif defined(CENTAURUS_BUILD_LINUX)
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include <stdio.h>

namespace Centaurus
{
#if defined(CENTAURUS_BUILD_WINDOWS)

#define PROGRAM_UUID L"{57DF45C9-6D0C-4DD2-9B41-B71F8CF66B13}"
#define PROGRAM_NAME L"Centaurus.exe"

class MasterASTRingBuffer
{
    HANDLE m_handle;
    void *m_buffer;
    size_t m_bank_size;
    int m_bank_num, m_current_bank;
public:
    MasterASTRingBuffer(size_t bank_size, int bank_num)
        : m_handle(INVALID_HANDLE_VALUE), m_buffer(NULL), m_bank_size(bank_size), m_bank_num(bank_num)
    {
        size_t buffer_size = bank_size * bank_num;

        m_handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, buffer_size, L"AST Buffer");

        m_buffer = MapViewOfFile(m_handle, FILE_MAP_ALL_ACCESS, 0, 0, buffer_size);
    }
    virtual ~MasterASTRingBuffer()
    {
        if (m_buffer != NULL)
            UnmapViewOfFile(m_buffer);
        if (m_handle != NULL)
            CloseHandle(m_handle);
    }
    void *get_buffer()
    {
        return m_buffer;
    }
};
#elif defined(CENTAURUS_BUILD_LINUX)

#define PROGRAM_UUID "{57DF45C9-6D0C-4DD2-9B41-B71F8CF66B13}"
#define PROGRAM_NAME "Centaurus"

class MasterASTRingBuffer
{
    void *m_buffer;
    size_t m_bank_size;
    int m_bank_num;
public:
    MasterASTRingBuffer(size_t bank_size, int bank_num)
        : m_buffer(NULL), m_bank_size(bank_size), m_bank_num(bank_num)
    {
        size_t buffer_size = bank_size * bank_num;

        char buf[256];

        snprintf(buf, sizeof(buf), "/Centaurus_%d_MasterBuffer", /*PROGRAM_UUID, PROGRAM_NAME, */getpid());

        int fd = shm_open(buf, O_RDWR | O_CREAT, 0666);

        ftruncate(fd, buffer_size);

        m_buffer = mmap(NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        //close(fd);
    }
    virtual ~MasterASTRingBuffer()
    {
        munmap(m_buffer, m_bank_size * m_bank_num);

        char buf[256];

        snprintf(buf, sizeof(buf), "/Centaurus_%d_MasterBuffer", /*PROGRAM_UUID, PROGRAM_NAME, */getpid());

        shm_unlink(buf);
    }
    void *get_buffer()
    {
        return m_buffer;
    }
};
#endif

/*class IPCMaster
{
    HANDLE m_pipe, m_semaphore;
public:
    IPCMaster()
    {
        wchar_t buf[MAX_PATH];

        _snwprintf(buf, MAX_PATH, LR"(\\.\pipe\%s%s[%u]Master)", PROGRAM_UUID, PROGRAM_NAME, GetCurrentProcessId());

        m_pipe = CreateNamedPipeW(buf, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, 4096, 4096, 1000, NULL);

        _snwprintf(buf, MAX_PATH, LR"()");

        m_semaphore = CreateSemaphore(NULL, 0, 256, );
    }
    virtual ~IPCMaster()
    {

    }
};*/
/*class SlaveASTRingBuffer
{
    HANDLE m_handle;
public:
    SlaveASTRingBuffer(LPCTSTR name)
    {
        m_handle = OpenFileMapping(PAGE_READONLY, FALSE, name);

        Assert::AreNotEqual((void *)NULL, (void *)m_handle);


    }
    virtual ~SlaveASTRingBuffer()
    {
        if (m_handle != NULL)
            CloseHandle(m_handle);
    }
};*/

}
