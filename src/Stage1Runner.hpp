#pragma once

#include "BaseRunner.hpp"

namespace Centaurus
{
class Stage1Runner : public BaseRunner
{
    IParser *m_parser;
    int m_current_bank, m_counter;
private:
#if defined(CENTAURUS_BUILD_WINDOWS)
    static DWORD WINAPI thread_runner(LPVOID param)
#elif defined(CENTAURUS_BUILD_LINUX)
    static void *thread_runner(void *param)
#endif
    {
		Stage1Runner *instance = reinterpret_cast<Stage1Runner *>(param);

        instance->m_current_bank = -1;
        instance->m_counter = 0;
        instance->reset_banks();

        std::cout << "Executing generated code." << std::endl;

        (*instance->m_parser)(static_cast<BaseListener *>(instance), instance->m_input_window);

        std::cout << "Finished running generated code." << std::endl;

        instance->release_bank();

        instance->signal_exit();

#if defined(CENTAURUS_BUILD_WINDOWS)
        ExitThread(0);
#elif defined(CENTAURUS_BUILD_LINUX)
		return NULL;
#endif
    }
    void *acquire_bank()
    {
        WindowBankEntry *banks = (WindowBankEntry *)m_sub_window;

		while (true)
		{
			for (int i = 0; i < m_bank_num; i++)
			{
				WindowBankState old_state = WindowBankState::Free;
				if (banks[i].state.compare_exchange_weak(old_state, WindowBankState::Stage1_Locked))
				{
					m_current_bank = i;
					return (char *)m_main_window + m_bank_size * i;
				}
			}
		}
    }
    void release_bank()
    {
        if (m_current_bank != -1)
        {
            WindowBankEntry *banks = (WindowBankEntry *)m_sub_window;

            banks[m_current_bank].number = m_counter++;
            banks[m_current_bank].state.store(WindowBankState::Stage1_Unlocked);

            m_current_bank = -1;

#if defined(CENTAURUS_BUILD_WINDOWS)
            ReleaseSemaphore(m_slave_lock, 1, NULL);
#elif defined(CENTAURUS_BUILD_LINUX)
            sem_post(m_slave_lock);
#endif
        }
    }
    void reset_banks()
    {
        WindowBankEntry *banks = (WindowBankEntry *)m_sub_window;

        for (int i = 0; i < m_bank_num; i++)
        {
            banks[i].state.store(WindowBankState::Free);
        }
    }
    void signal_exit()
    {
        WindowBankEntry *banks = (WindowBankEntry *)m_sub_window;

        while (true)
        {
            int count = 0;
            for (int i = 0; i < m_bank_num; i++)
            {
                WindowBankState state = banks[i].state.load();

                if (state != WindowBankState::Free)
                    count++;
            }
            if (count == 0) break;
        }

        for (int i = 0; i < m_bank_num; i++)
        {
            banks[i].state.store(WindowBankState::YouAreDone);
        
#if defined(CENTAURUS_BUILD_WINDOWS)
            ReleaseSemaphore(m_slave_lock, 1, NULL);
#elif defined(CENTAURUS_BUILD_LINUX)
            sem_post(m_slave_lock);
#endif
        }
    }
public:
    Stage1Runner(const char *filename, IParser *parser, size_t bank_size, int bank_num)
#if defined(CENTAURUS_BUILD_WINDOWS)
        : BaseRunner(filename, bank_size, bank_num, GetCurrentProcessId()),
#elif defined(CENTAURUS_BUILD_LINUX)
		: BaseRunner(filename, bank_size, bank_num, getpid()),
#endif
		m_parser(parser)
    {
#if defined(CENTAURUS_BUILD_WINDOWS)
        m_mem_handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, get_window_size(), m_memory_name);

		m_sub_window = MapViewOfFile(m_mem_handle, FILE_MAP_ALL_ACCESS, 0, 0, get_sub_window_size());

        m_main_window = MapViewOfFile(m_mem_handle, FILE_MAP_ALL_ACCESS, 0, get_sub_window_size(), get_main_window_size());

		m_slave_lock = CreateSemaphoreExA(NULL, 0, bank_num, m_slave_lock_name, 0, SEMAPHORE_MODIFY_STATE);
#elif defined(CENTAURUS_BUILD_LINUX)
        int fd = shm_open(m_memory_name, O_RDWR | O_CREAT, 0600);

        ftruncate(fd, get_window_size());

		m_sub_window = mmap(NULL, get_sub_window_size(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        m_main_window = mmap(NULL, get_main_window_size(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, get_sub_window_size());

        close(fd);

		m_slave_lock = sem_open(m_slave_lock_name, O_CREAT | O_EXCL, 0600, 0);
#endif
    }
	~Stage1Runner()
	{
#if defined(CENTAURUS_BUILD_WINDOWS)
        if (m_slave_lock != NULL)
            CloseHandle(m_slave_lock);
        if (m_main_window != NULL)
            UnmapViewOfFile(m_main_window);
		if (m_sub_window != NULL)
			UnmapViewOfFile(m_sub_window);
		if (m_mem_handle != INVALID_HANDLE_VALUE)
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
    virtual void start() override
    {
        _start(Stage1Runner::thread_runner, static_cast<void *>(this));
    }
    virtual void *feed_callback() override
    {
        release_bank();
        return acquire_bank();
    }
};
}
