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

#define ALIGN_NEXT(x, a) (((x) + (a) - 1) / (a) * (a))
#define PROGRAM_UUID "{57DF45C9-6D0C-4DD2-9B41-B71F8CF66B13}"
#define PROGRAM_NAME "Centaurus"
#define IPC_PAGESIZE (8 * 1024 * 1024)

namespace Centaurus
{
class IPCBase
{
protected:
	struct ASTBankEntry
	{
		int flag;
		int number;
	};
    void *m_main_window, *m_sub_window;
    size_t m_bank_size;
	int m_bank_num;
#if defined(CENTAURUS_BUILD_WINDOWS)
    HANDLE m_mem_handle, m_lock_handle;
#elif defined(CENTAURUS_BUILD_LINUX)
	sem_t *m_lock;
#endif
#if defined(CENTAURUS_BUILD_WINDOWS)
	char m_memory_name[MAX_PATH], m_lock_name[MAX_PATH];
#elif defined(CENTAURUS_BUILD_LINUX)
	char m_memory_name[256], m_lock_name[256];
#endif
public:
	IPCBase(size_t bank_size, int bank_num, int pid)
		: m_bank_size(bank_size), m_bank_num(bank_num)
	{
#if defined(CENTAURUS_BUILD_WINDOWS)
		sprintf_s(m_memory_name, "%s%s[%u]Memory", PROGRAM_UUID, PROGRAM_NAME, pid);

		sprintf_s(m_lock_name, "%s%s[%u]Lock", PROGRAM_UUID, PROGRAM_NAME, pid);
#elif defined(CENTAURUS_BUILD_LINUX)
        snprintf(m_memory_name, sizeof(m_memory_name), "/%s%s[%d]Memory", PROGRAM_UUID, PROGRAM_NAME, pid);

		snprintf(m_lock_name, sizeof(m_lock_name), "/%s%s[%d]Lock", PROGRAM_UUID, PROGRAM_NAME, pid);
#endif
	}
	virtual ~IPCBase()
	{
	}
protected:
	size_t get_main_window_size()
	{
		return ALIGN_NEXT(m_bank_size, IPC_PAGESIZE) * m_bank_num;
	}
	size_t get_sub_window_size()
	{
		return ALIGN_NEXT(sizeof(ASTBankEntry) * m_bank_num, IPC_PAGESIZE);
	}
	size_t get_window_size()
	{
		return get_main_window_size() + get_sub_window_size();
	}
};
}
