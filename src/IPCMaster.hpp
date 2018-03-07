#pragma once

#if defined(CENTAURUS_BUILD_WINDOWS)
#include <Windows.h>
#elif defined(CENTAURUS_BUILD_LINUX)
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#endif

#include <stdio.h>

#define PROGRAM_UUID "{57DF45C9-6D0C-4DD2-9B41-B71F8CF66B13}"
#define PROGRAM_NAME "Centaurus"

namespace Centaurus
{
class IPCMaster
{
	struct ASTBankEntry
	{
		int flag;
		int number;
	};

    void *m_buffer;
	ASTBankEntry *m_table;
    size_t m_bank_size;
    int m_bank_num, m_current_bank;
#if defined(CENTAURUS_BUILD_WINDOWS)
    HANDLE m_main_mem_handle, m_sub_mem_handle, m_lock_handle;
#elif defined(CENTAURUS_BUILD_LINUX)
	sem_t m_lock;
#endif
public:
    IPCMaster(size_t bank_size, int bank_num)
        : m_bank_size(bank_size), m_bank_num(bank_num)
    {
        size_t main_mem_size = bank_size * bank_num;
		size_t sub_mem_size = ALIGN_NEXT(sizeof(ASTBankEntry) * bank_num);

#if defined(CENTAURUS_BUILD_WINDOWS)
		char name[MAX_PATH];

		sprintf_s(name, "%s%s[%u]Buffer", PROGRAM_UUID, PROGRAM_NAME, GetCurrentProcessId());

        m_mem_handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, buffer_size, name);

        m_buffer = MapViewOfFile(m_mem_handle, FILE_MAP_ALL_ACCESS, 0, 0, buffer_size);

		sprintf_s(name, "%s%s[%u]Lock", PROGRAM_UUID, PROGRAM_NAME, GetCurrentProcessId());

		m_lock_handle = CreateMutexA(NULL, FALSE, name);
#elif defined(CENTAURUS_BUILD_LINUX)
        char name[256];

        snprintf(name, sizeof(name), "/%s%s[%d]Buffer", PROGRAM_UUID, PROGRAM_NAME, getpid());

        int fd = shm_open(name, O_RDWR | O_CREAT, 0666);

        ftruncate(fd, buffer_size);

        m_buffer = mmap(NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        close(fd);

		snprintf(name, sizeof(name), "/%s%s[%d]Lock", PROGRAM_UUID, PROGRAM_NAME, getpid());

		m_lock = sem_open(name, O_CREAT | O_EXCL, 0600, 0);
#endif
    }
    virtual ~IPCMaster()
    {
#if defined(CENTAURUS_BUILD_WINDOWS)
		if (m_lock_handle != NULL)
			CloseHandle(m_lock_handle);
        if (m_buffer != NULL)
            UnmapViewOfFile(m_buffer);
        if (m_mem_handle != NULL)
            CloseHandle(m_mem_handle);
#elif defined(CENTAURUS_BUILD_LINUX)
		size_t buffer_size = m_bank_size * m_bank_num + sizeof(ASTBankEntry) * m_bank_num;

		if (m_buffer != NULL)
			munmap(m_buffer, buffer_size);

        char name[256];

        snprintf(name, sizeof(name), "/%s%s[%d]Buffer", PROGRAM_UUID, PROGRAM_NAME, getpid());

        shm_unlink(name);

		snprintf(name, sizeof(name), "/%s%s[%d]Lock", PROGRAM_UUID, PROGRAM_NAME, getpid());

		sem_unlink(name);

		if (m_lock != SEM_FAILED)
			sem_close(m_lock);
#endif
    }
    void *get_buffer()
    {
        return m_buffer;
    }
};
}
