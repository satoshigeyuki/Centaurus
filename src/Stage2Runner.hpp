#pragma once

#include <stdint.h>
#include <time.h>
#include "BaseRunner.hpp"
#include "CodeGenEM64T.hpp"

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

        instance->m_current_bank = -1;

		while (true)
		{
            uint64_t *data = reinterpret_cast<uint64_t *>(instance->acquire_bank());

            if (data == NULL) break;

            instance->reduce_bank(data);

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
            else if (ast[i] == 0)
            {
                break;
            }
		}
	}
	int parse_subtree(uint64_t *ast, int position)
	{
		CSTMarker start_marker(ast[position]);
		int i, j;
        j = position + 1;
		for (i = position + 1; i < m_bank_size / 8; i++)
		{
            if (ast[i] == 0) return m_bank_size / 8;
			CSTMarker marker(ast[i]);
			if (marker.is_start_marker())
			{
                int k = parse_subtree(ast, i);
                ast[j] = ast[i];
                ast[j + 1] = ast[i + 1];
                j += 2;
                i = k;
			}
            else if (marker.is_end_marker())
            {
                m_chaser[marker.get_machine_id()](this, start_marker.offset_ptr(m_input_window));
                ast[position] = ((uint64_t)1 << 63) | marker.get_offset();
                //ToDo: store SV pointer to the second entry
                ast[position + 1] = NULL;
                return i + 1;
            }
		}
		return i;
	}
    void *acquire_bank()
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
					return (char *)m_main_window + m_bank_size * i;
				}
                else
                {
                    if (old_state == WindowBankState::YouAreDone)
                    {
                        return NULL;
                    }
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
        : BaseRunner(filename, bank_size, bank_num, master_pid), m_chaser(chaser), m_bank_size(bank_size)
    {
#if defined(CENTAURUS_BUILD_WINDOWS)
		m_mem_handle = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, m_memory_name);

		m_sub_window = MapViewOfFile(m_mem_handle, FILE_MAP_ALL_ACCESS, 0, 0, get_sub_window_size());

		m_main_window = MapViewOfFile(m_mem_handle, FILE_MAP_ALL_ACCESS, 0, get_sub_window_size(), get_main_window_size());

		m_slave_lock = OpenSemaphoreA(SYNCHRONIZE, FALSE, m_slave_lock_name);
#elif defined(CENTAURUS_BUILD_LINUX)
		int fd = shm_open(m_memory_name, O_RDWR, 0666);

		ftruncate(fd, get_window_size());

		m_sub_window = mmap(NULL, get_sub_window_size(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

		m_main_window = mmap(NULL, get_main_window_size(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, get_sub_window_size());

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
        _start(Stage2Runner<T>::thread_runner, this);
    }
    virtual void terminal_callback(int id, const void *start, const void *end) override
    {
        //Do nothing
    }
    virtual const void *nonterminal_callback(int id, const void *input) override
    {
        return NULL;
    }
};
}
