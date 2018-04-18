#pragma once

#include <stdint.h>
#include <time.h>
#include "BaseRunner.hpp"
#include "CodeGenEM64T.hpp"

#define STAGE2_STACK_SIZE (64 * 1024 * 1024)

namespace Centaurus
{
template<class T>
class Stage2Runner : public BaseRunner
{
    size_t m_bank_size;
    T& m_chaser;
	int m_current_bank;
private:
#if defined(CENTAURUS_BUILD_WINDOWS)
	static DWORD WINAPI thread_runner(LPVOID param)
#elif defined(CENTAURUS_BUILD_LINUX)
	static void *thread_runner(void *param)
#endif
	{
		Stage2Runner<T> *instance = reinterpret_cast<Stage2Runner<T> *>(param);

		while (true)
		{
			instance->reduce_bank(reinterpret_cast<const uint64_t *>(instance->acquire_bank()));
            instance->release_bank();
		}

#if defined(CENTAURUS_BUILD_WINDOWS)
		ExitThread(0);
#elif defined(CENTAURUS_BUILD_LINUX)
		return NULL;
#endif
	}
	void reduce_bank(uint64_t *ast)
	{
		for (int i = 0; i < m_bank_size / 8; i++)
		{
			CSTMarker marker(ast[i]);
			if (marker.is_start_marker())
			{
				i = parse_subtree(ast, i);
			}
		}
	}
	int parse_subtree(uint64_t *ast, int position)
	{
		CSTMarker start_marker(ast[position]);
		int i;
		for (i = position + 1; i < m_bank_size / 8; i++)
		{
			CSTMarker marker(ast[i]);
			if (marker.is_start_marker())
			{
                i = parse_subtree(ast, i);
			}
            else if (marker.is_end_marker())
            {
				//ToDo: Invoke chaser to derive the semantic value for this symbol
				return i + 1;
            }
		}
		return i;
	}
    const void *acquire_bank()
    {
#if defined(CENTAURUS_BUILD_WINDOWS)
        WaitForSingleObject(m_slave_lock, INFINITE);
#elif defined(CENTAURUS_BUILD_LINUX)
        sem_wait(m_slave_lock);
#endif
        WindowBankEntry *banks = reinterpret_cast<WindowBankEntry *>(m_sub_window);

		while (true)
		{
			for (int i = 0; i < m_bank_num; i++)
			{
				WindowBankState old_state = WindowBankState::Stage1_Unlocked;

				if (banks[i].state.compare_exchange_weak(old_state, WindowBankState::Stage2_Locked))
				{
					m_current_bank = i;
					return (const char *)m_main_window + m_bank_size * i;
				}
			}
		}
    }
	void release_bank()
	{
		WindowBankEntry *banks = reinterpret_cast<WindowBankEntry *>(m_sub_window);

		banks[m_current_bank].state.store(WindowBankState::Stage2_Unlocked);

		m_current_bank = -1;
	}
public:
    Stage2Runner(const char *filename, T& chaser, size_t bank_size, int bank_num, int master_pid)
        : BaseRunner(filename, bank_size, bank_num, master_pid, STAGE2_STACK_SIZE), m_chaser(chaser), m_bank_size(bank_size), m_current_bank(-1)
    {
#if defined(CENTAURUS_BUILD_WINDOWS)
		m_mem_handle = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, m_memory_name);

		m_sub_window = MapViewOfFile(m_mem_handle, FILE_MAP_ALL_ACCESS, 0, 0, get_sub_window_size());

		m_main_window = MapViewOfFile(m_mem_handle, FILE_MAP_READ, 0, get_sub_window_size(), get_main_window_size());

		m_slave_lock = OpenSemaphoreA(SYNCHRONIZE, FALSE, m_slave_lock_name);
#elif defined(CENTAURUS_BUILD_LINUX)
		int fd = shm_open(m_memory_name, O_RDWR, 0666);

		ftruncate(fd, get_window_size());

		m_sub_window = mmap(NULL, get_sub_window_size(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

		m_main_window = mmap(NULL, get_main_window_size(), PROT_READ, MAP_SHARED, fd, get_sub_window_size());

		m_slave_lock = sem_open(m_slave_lock_name, 0);
#endif
    }
    virtual ~Stage2Runner()
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
        if (m_slave_lock != SEM_FAILED)
            sem_close(m_slave_lock);
#endif
    }
	void start()
	{
        start(Stage2Runner<T>::thread_runner);
    }
	void run()
	{
		start();
		wait();
	}
};
}
