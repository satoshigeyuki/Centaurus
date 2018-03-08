#pragma once

#include "IPCCommon.hpp"

namespace Centaurus
{
class IPCSlave : public IPCCommon
{
public:
	IPCSlave(size_t bank_size, int bank_num, int master_pid)
		: IPCCommon(bank_size, bank_num, master_pid)
	{
#if defined(CENTAURUS_BUILD_WINDOWS)
		m_mem_handle = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, m_memory_name);

		m_sub_window = MapViewOfFile(m_mem_handle, FILE_MAP_ALL_ACCESS, 0, 0, get_sub_window_size());

		m_main_window = MapViewOfFile(m_mem_handle, FILE_MAP_READ, 0, get_sub_window_size(), get_main_window_size());

		m_lock_handle = OpenMutexA(SYNCHRONIZE, FALSE, m_lock_name);
#elif defined(CENTAURUS_BUILD_LINUX)
		int fd = shm_open(m_memory_name, O_RDWR, 0666);

		ftruncate(fd, get_window_size());

		m_sub_window = mmap(NULL, get_sub_window_size(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

		m_main_window = mmap(NULL, get_main_window_size(), PROT_READ, MAP_SHARED, fd, get_sub_window_size());

		m_lock = sem_open(m_lock_name, 0);
#endif
	}
	virtual ~IPCSlave()
	{
#if defined(CENTAURUS_BUILD_WINDOWS)
		if (m_lock_handle != NULL)
			CloseHandle(m_lock_handle);
		if (m_main_window != NULL)
			UnmapViewOfFile(m_main_window);
		if (m_sub_window != NULL)
			UnmapViewOfFile(m_sub_window);
		if (m_mem_handle != NULL)
			CloseHandle(m_mem_handle);
#elif defined(CENTAURUS_BUILD_LINUX)
		if (m_main_window != MAP_FAILED)
			munmap(m_main_window, get_main_window_size());
		if (m_sub_window != MAP_FAILED)
			munmap(m_sub_window, get_sub_window_size());
		if (m_lock != SEM_FAILED)
			sem_close(m_lock);
#endif
	}
};
}
