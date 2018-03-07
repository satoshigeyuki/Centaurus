#pragma once

#if defined(CENTAURUS_BUILD_WINDOWS)
#include <Windows.h>
#elif defined(CENTAURUS_BUILD_LINUX)
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <mqueue.h>
#include <semaphore.h>
#endif

#define ALIGN_NEXT(x, a) (((x) + (a) - 1) / (a) * (a))

#include <stdio.h>

#define PROGRAM_UUID "{57DF45C9-6D0C-4DD2-9B41-B71F8CF66B13}"
#define PROGRAM_NAME "Centaurus"

namespace Centaurus
{
class IPCSlave
{
	struct ASTBankEntry
	{
		int flag;
		int number;
	};

	void *m_buffer;
	ASTBankEntry *m_table;
	size_t m_bank_size;
	int m_bank_num;
#if defined(CENTAURUS_BUILD_WINDOWS)
	HANDLE m_main_mem_handle, m_sub_mem_handle, m_lock_handle;
#elif defined(CENTAURUS_BUILD_LINUX)
	sem_t m_lock;
#endif
public:
	IPCSlave(size_t bank_size, int bank_num, int master_pid)
		: m_bank_size(bank_size), m_bank_num(bank_num)
	{
		size_t m_buffer_size = bank_size * bank_num;

#if defined(CENTAURUS_BUILD_WINDOWS)
		char name[MAX_PATH];

		sprintf_s(name, "%s%s[%u]Buffer", PROGRAM_UUID, PROGRAM_NAME, master_pid);

		m_mem_handle = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name);

		m_buffer = MapViewOfFile(m_mem_handle, FILE_MAP_ALL_ACCESS, 0, 0, buffer_size);

		sprintf_s(name, "%s%s[%u]Lock", PROGRAM_UUID, PROGRAM_NAME, master_pid);

		m_lock_handle = OpenMutexA(SYNCHRONIZE, FALSE, name);
#elif defined(CENTAURUS_BUILD_LINUX)
		char name[256];

		snprintf(name, "/%s%s[%u]Buffer", PROGRAM_UUID, PROGRAM_NAME, master_pid);

		int fd = shm_open(name, O_RDWR, 0666);


#endif
	}
	virtual ~IPCSlave()
	{
		if (m_lock_handle != NULL)
			CloseHandle(m_lock_handle);
		if (m_buffer != NULL)
			UnmapViewOfFile(m_buffer);
		if (m_mem_handle != NULL)
			CloseHandle(m_mem_handle);
	}
};
}
