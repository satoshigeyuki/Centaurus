#pragma once

#include "IPCCommon.hpp"

namespace Centaurus
{
class IPCMaster : public IPCBase
{
    int m_current_bank;
public:
    IPCMaster(size_t bank_size, int bank_num)
#if defined(CENTAURUS_BUILD_WINDOWS)
        : IPCBase(bank_size, bank_num, GetCurrentProcessId())
#elif defined(CENTAURUS_BUILD_LINUX)
		: IPCBase(bank_size, bank_num, getpid())
#endif
    {
#if defined(CENTAURUS_BUILD_WINDOWS)
        m_mem_handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, get_window_size(), m_memory_name);

		m_sub_window = MapViewOfFile(m_mem_handle, FILE_MAP_ALL_ACCESS, 0, 0, get_sub_window_size());

        m_main_window = MapViewOfFile(m_mem_handle, FILE_MAP_ALL_ACCESS, 0, get_sub_window_size(), get_main_window_size());

		m_lock_handle = CreateMutexA(NULL, FALSE, m_lock_name);
#elif defined(CENTAURUS_BUILD_LINUX)
        int fd = shm_open(m_memory_name, O_RDWR | O_CREAT, 0600);

        ftruncate(fd, get_window_size());

		m_sub_window = mmap(NULL, get_sub_window_size(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        m_main_window = mmap(NULL, get_main_window_size(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, get_sub_window_size());

        close(fd);

		m_lock = sem_open(m_lock_name, O_CREAT | O_EXCL, 0600, 0);
#endif
    }
    virtual ~IPCMaster()
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

        shm_unlink(m_memory_name);

		sem_unlink(m_lock_name);

		if (m_lock != SEM_FAILED)
			sem_close(m_lock);
#endif
    }
    void *get_buffer()
    {
        return m_main_window;
    }
};
}
