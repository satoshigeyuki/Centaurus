#pragma once

#include "IPCCommon.hpp"

namespace Centaurus
{
class IPCMaster : public IPCBase
{
    int m_current_bank, m_counter;
public:
    IPCMaster(size_t bank_size, int bank_num)
#if defined(CENTAURUS_BUILD_WINDOWS)
        : IPCBase(bank_size, bank_num, GetCurrentProcessId()), m_current_bank(-1), m_counter(0)
#elif defined(CENTAURUS_BUILD_LINUX)
		: IPCBase(bank_size, bank_num, getpid())
#endif
    {
#if defined(CENTAURUS_BUILD_WINDOWS)
        m_mem_handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, get_window_size(), m_memory_name);

		m_sub_window = MapViewOfFile(m_mem_handle, FILE_MAP_ALL_ACCESS, 0, 0, get_sub_window_size());

        m_main_window = MapViewOfFile(m_mem_handle, FILE_MAP_ALL_ACCESS, 0, get_sub_window_size(), get_main_window_size());

		m_slave_lock = CreateSemaphoreA(NULL, 0, bank_num, m_slave_lock_name);

        m_master_lock = CreateSemaphoreA(NULL, bank_num, bank_num, m_master_lock_name);

        m_window_lock = CreateMutexA(NULL, FALSE, m_window_lock_name);
#elif defined(CENTAURUS_BUILD_LINUX)
        int fd = shm_open(m_memory_name, O_RDWR | O_CREAT, 0600);

        ftruncate(fd, get_window_size());

		m_sub_window = mmap(NULL, get_sub_window_size(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        m_main_window = mmap(NULL, get_main_window_size(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, get_sub_window_size());

        close(fd);

		m_slave_lock = sem_open(m_slave_lock_name, O_CREAT | O_EXCL, 0600, 0);

        m_master_lock = sem_open(m_master_lock_name, O_CREAT | O_EXCL, 0600, 0);

        for (int i = 0; i < bank_num; i++)
            sem_post(m_master_lock);

        m_window_lock = sem_open(m_window_lock_name, O_CREAT | O_EXCL, 0600, 0);
#endif
    }
    virtual ~IPCMaster()
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

        shm_unlink(m_memory_name);

		sem_unlink(m_window_lock_name);
        sem_unlink(m_master_lock_name);
        sem_unlink(m_slave_lock_name);

		if (m_window_lock != SEM_FAILED)
			sem_close(m_window_lock);
        if (m_master_lock != SEM_FAILED)
            sem_close(m_master_lock);
        if (m_slave_lock != SEM_FAILED)
            sem_close(m_slave_lock);
#endif
    }
    void reset_counter()
    {
        m_counter = 0;
    }
    void *request_bank()
    {
#if defined(CENTAURUS_BUILD_WINDOWS)
        WaitForSingleObject(m_master_lock, INFINITE);
#elif defined(CENTAURUS_BUILD_LINUX)
        sem_wait(m_master_lock);
#endif
        ASTBankEntry *banks = (ASTBankEntry *)m_sub_window;

        int i;
        for (i = 0; i < m_bank_num; i++)
        {
            //Leave the new bank unlocked, because nobody else will try to use it.
            if (banks[i].number.load() < 0)
                break;
        }
        m_current_bank = i;
        return (char *)m_main_window + m_bank_size * i;
    }
    void dispatch_bank()
    {
        ASTBankEntry *banks = (ASTBankEntry *)m_sub_window;

        banks[m_current_bank].number.store(m_counter++);

        m_current_bank = -1;

#if defined(CENTAURUS_BUILD_WINDOWS)
        ReleaseSemaphore(m_slave_lock, 1, NULL);
#elif defined(CENTAURUS_BUILD_LINUX)
        sem_post(m_slave_lock);
#endif
    }
    static void *exchange_bank(void *context)
    {
        IPCMaster *instance = reinterpret_cast<IPCMaster *>(context);
        instance->dispatch_bank();
        return instance->request_bank();
    }
};
}
