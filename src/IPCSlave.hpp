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

		m_slave_lock = OpenSemaphoreA(SYNCHRONIZE, FALSE, m_slave_lock_name);

        m_master_lock = OpenSemaphoreA(SYNCHRONIZE, FALSE, m_master_lock_name);

        m_window_lock = OpenMutexA(SYNCHRONIZE, FALSE, m_window_lock_name);
#elif defined(CENTAURUS_BUILD_LINUX)
		int fd = shm_open(m_memory_name, O_RDWR, 0666);

		ftruncate(fd, get_window_size());

		m_sub_window = mmap(NULL, get_sub_window_size(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

		m_main_window = mmap(NULL, get_main_window_size(), PROT_READ, MAP_SHARED, fd, get_sub_window_size());

		m_slave_lock = sem_open(m_slave_lock_name, 0);

        m_master_lock = sem_open(m_master_lock_name, 0);

        m_window_lock = sem_open(m_window_lock_name, 0);
#endif
	}
	virtual ~IPCSlave()
	{
#if defined(CENTAURUS_BUILD_WINDOWS)
		if (m_window_lock != NULL)
			CloseHandle(m_window_lock);
        if (m_master_lock != NULL)
            CloseHandle(m_master_lock);
        if (m_slave_lock != NULL)
            CloseHandle(m_slave_lock);
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
		if (m_window_lock != SEM_FAILED)
			sem_close(m_window_lock);
        if (m_master_lock != SEM_FAILED)
            sem_close(m_master_lock);
        if (m_slave_lock != SEM_FAILED)
            sem_close(m_slave_lock);
#endif
	}
    std::pair<const void *, int> request_bank()
    {
#if defined(CENTAURUS_BUILD_WINDOWS)
        WaitForSingleObject(m_slave_lock, INFINITE);
        WaitForSingleObject(m_window_lock, INFINITE);
#elif defined(CENTAURUS_BUILD_LINUX)
        sem_wait(m_slave_lock);
        sem_wait(m_window_lock);
#endif
        ASTBankEntry *banks = reinterpret_cast<ASTBankEntry *>(m_sub_window);
        int i, number = -1;
        for (i = 0; i < m_bank_num; i++)
        {
            number = banks[i].number.exchange(-1);
            if (number != -1)
                break;
        }
#if defined(CENTAURUS_BUILD_WINDOWS)
        ReleaseMutex(m_window_lock);
        ReleaseSemaphore(m_slave_lock, 1, NULL);
#elif defined(CENTAURUS_BUILD_LINUX)
        sem_post(m_window_lock);
        sem_post(m_slave_lock);
#endif
        const void *ptr = (const char *)m_main_window + m_bank_size * i;

        return std::pair<const void *, int>(ptr, number);
    }
};
}
