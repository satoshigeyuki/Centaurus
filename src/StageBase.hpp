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
#include <atomic>

#define ALIGN_NEXT(x, a) (((x) + (a) - 1) / (a) * (a))
#define PROGRAM_UUID "{57DF45C9-6D0C-4DD2-9B41-B71F8CF66B13}"
#define PROGRAM_NAME "Centaurus"
#define IPC_PAGESIZE (8 * 1024 * 1024)

namespace Centaurus
{
class StageBase
{
protected:
	enum class ASTBankState
	{
		Free,
		Stage1_Locked,
		Stage1_Unlocked,
		Stage2_Locked,
		Stage2_Unlocked,
		Stage3_Locked,
		Stage3_Unlocked
	};
	struct ASTBankEntry
	{
		int number;
		std::atomic<ASTBankState> state;
	};
	const void *m_input_window;
    void *m_main_window, *m_sub_window;
    size_t m_bank_size;
	int m_bank_num;
#if defined(CENTAURUS_BUILD_WINDOWS)
    HANDLE m_input_handle, m_mem_handle, m_slave_lock;
#elif defined(CENTAURUS_BUILD_LINUX)
	sem_t *m_slave_lock;
#endif
#if defined(CENTAURUS_BUILD_WINDOWS)
	char m_memory_name[MAX_PATH], m_slave_lock_name[MAX_PATH];
#elif defined(CENTAURUS_BUILD_LINUX)
	char m_memory_name[256], m_slave_lock_name[256];
#endif
public:
	IPCBase(const char *filename, size_t bank_size, int bank_num, int pid)
		: m_bank_size(bank_size), m_bank_num(bank_num)
	{
#if defined(CENTAURUS_BUILD_WINDOWS)
		HANDLE input_file_handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

		m_input_window = CreateFileMappingA(input_file_handle, NULL, PAGE_READONLY, 

		sprintf_s(m_memory_name, "%s%s[%u]Window", PROGRAM_UUID, PROGRAM_NAME, pid);

		sprintf_s(m_slave_lock_name, "%s%s[%u]SlaveLock", PROGRAM_UUID, PROGRAM_NAME, pid);
#elif defined(CENTAURUS_BUILD_LINUX)
        snprintf(m_memory_name, sizeof(m_memory_name), "/%s%s[%d]Window", PROGRAM_UUID, PROGRAM_NAME, pid);

		snprintf(m_slave_lock_name, sizeof(m_slave_lock_name), "/%s%s[%d]SlaveLock", PROGRAM_UUID, PROGRAM_NAME, pid);
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
