#pragma once

#if defined(CENTAURUS_BUILD_WINDOWS)
#include <Windows.h>
#elif defined(CENTAURUS_BUILD_LINUX)
#include <pthread.h>
#include <unistd.h>
#endif
#include "IPCCommon.hpp"

namespace Centaurus
{
template<class T>
class Stage1Parser : public IPCBase
{
	static constexpr size_t m_stack_size = 256 * 1024 * 1024;
    T& m_parser;
    int m_current_bank, m_counter;
#if defined(CENTAURUS_BUILD_WINDOWS)
	HANDLE m_thread;
#elif defined(CENTAURUS_BUILD_LINUX)
	pthread_t m_thread;
#endif
private:
#if defined(CENTAURUS_BUILD_WINDOWS)
    static DWORD WINAPI thread_runner(LPVOID param)
    {
        Stage1Parser<T> *instance = reinterpret_cast<Stage1Parser<T> *>(param);

        instance->m_result = instance->m_parser(instance->m_memory);

        ExitThread(0);
    }
#elif defined(CENTAURUS_BUILD_LINUX)
    static void *thread_runner(void *param)
    {
        Stage1Parser<T> *instance = reinterpret_cast<Stage1Parser<T> *>(param);

        instance->m_result = instance->m_parser(instance->m_memory);

		return NULL;
    }
#endif
    void *acquire_bank()
    {
        ASTBankEntry *banks = (ASTBankEntry *)m_sub_window;

		while (true)
		{
			for (int i = 0; i < m_bank_num; i++)
			{
				ASTBankState old_state = ASTBankState::Free;
				if (banks[i].state.compare_exchange_weak(old_state, ASTBankState::Stage1_Locked))
				{
					m_current_bank = i;
					return (char *)m_main_window + m_bank_size * i;
				}
			}
		}
    }
    void release_bank()
    {
        ASTBankEntry *banks = (ASTBankEntry *)m_sub_window;

        banks[m_current_bank].number = m_counter++;
		banks[m_current_bank].state.store(ASTBankState::Stage1_Unlocked);

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
        instance->release_bank();
        return instance->acquire_bank();
    }
public:
    Stage1Parser(T& parser, size_t bank_size, int bank_num)
#if defined(CENTAURUS_BUILD_WINDOWS)
        : IPCBase(bank_size, bank_num, GetCurrentProcessId()),
#elif defined(CENTAURUS_BUILD_LINUX)
		: IPCBase(bank_size, bank_num, getpid()),
#endif
		m_parser(parser), m_current_bank(-1), m_counter(0)
    {
#if defined(CENTAURUS_BUILD_WINDOWS)
        m_mem_handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, get_window_size(), m_memory_name);

		m_sub_window = MapViewOfFile(m_mem_handle, FILE_MAP_ALL_ACCESS, 0, 0, get_sub_window_size());

        m_main_window = MapViewOfFile(m_mem_handle, FILE_MAP_ALL_ACCESS, 0, get_sub_window_size(), get_main_window_size());

		m_slave_lock = CreateSemaphoreA(NULL, 0, bank_num, m_slave_lock_name);
#elif defined(CENTAURUS_BUILD_LINUX)
        int fd = shm_open(m_memory_name, O_RDWR | O_CREAT, 0600);

        ftruncate(fd, get_window_size());

		m_sub_window = mmap(NULL, get_sub_window_size(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        m_main_window = mmap(NULL, get_main_window_size(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, get_sub_window_size());

        close(fd);

		m_slave_lock = sem_open(m_slave_lock_name, O_CREAT | O_EXCL, 0600, 0);
#endif
    }
	~Stage1Parser()
	{
#if defined(CENTAURUS_BUILD_WINDOWS)
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

        sem_unlink(m_slave_lock_name);

        if (m_slave_lock != SEM_FAILED)
            sem_close(m_slave_lock);
#endif
	}
    void start()
    {
        clock_t start_time = clock();

#if defined(CENTAURUS_BUILD_WINDOWS)
        HANDLE m_thread = CreateThread(NULL, m_stack_size, MasterParser<T>::thread_runner, (LPVOID)this, 0, NULL);
#elif defined(CENTAURUS_BUILD_LINUX)
        pthread_t thread;
        pthread_attr_t attr;

        pthread_attr_init(&attr);

        pthread_attr_setstacksize(&attr, m_stack_size);

        pthread_create(&m_thread, &attr, MasterParser<T>::thread_runner, this);
#endif

        clock_t end_time = clock();

        //char buf[64];
        //snprintf(buf, 64, "Elapsed time = %lf[ms]\r\n", (double)(end_time - start_time) * 1000.0 / CLOCKS_PER_SEC);
        //Logger::WriteMessage(buf);
    }
	void wait()
	{
#if defined(CENTAURUS_BUILD_WINDOWS)
		WaitForSingleObject(m_thread, INFINITE);
#elif defined(CENTAURUS_BUILD_LINUX)
		pthread_join(m_thread, NULL);
#endif
	}
	void run()
	{
		start();
		wait();
	}
};
}
