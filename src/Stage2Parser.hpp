#pragma once

#if defined(CENTAURUS_BUILD_WINDOWS)
#include <Windows.h>
#elif defined(CENTAURUS_BUILD_LINUX)
#include <unistd.h>
#endif

#include <stdint.h>
#include <time.h>
#include "IPCCommon.hpp"
#include "CodeGenEM64T.hpp"

namespace Centaurus
{
class CSTMarker
{
	uint64_t m_value;
public:
	CSTMarker(uint64_t value) : m_value(value) {}
	int get_machine_id() const
	{
		return (m_value >> 48) & 0x7FFF;
	}
	bool is_start_marker() const
	{
		return m_value & ((uint64_t)1 << 63);
	}
	bool is_end_marker() const
	{
		return !(m_value & ((uint64_t)1 << 63));
	}
	uint64_t get_offset() const
	{
		return m_value & (((uint64_t)1 << 48) - 1);
	}
	const void *offset_ptr(const void *p) const
	{
		return static_cast<void *>((char *)p + get_offset());
	}
};
template<class T>
class Stage2Parser : public IPCBase
{
	static constexpr size_t m_stack_size = 64 * 1024 * 1024;
    size_t m_bank_size;
    T& m_chaser;
	const void *m_input;
	int m_current_bank;
#if defined(CENTAURUS_BUILD_WINDOWS)
	HANDLE m_thread;
#elif defined(CENTAURUS_BUILD_LINUX)
	pthread_t m_thread;
#endif
private:
#if defined(CENTAURUS_BUILD_WINDOWS)
	static DWORD WINAPI thread_runner(LPVOID param)
#elif defined(CENTAURUS_BUILD_LINUX)
	static void *thread_runner(void *param)
#endif
	{
		Stage2Parser<T> *instance = reinterpret_cast<Stage2Parser<T> *>(param);

		while (true)
		{
			reduce_bank(reinterpret_cast<const uint64_t *>(instance->m_ipc.acquire_bank()));
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
        ASTBankEntry *banks = reinterpret_cast<ASTBankEntry *>(m_sub_window);

		while (true)
		{
			for (int i = 0; i < m_bank_num; i++)
			{
				ASTBankState old_state = ASTBankState::Stage1_Unlocked;
				if (banks[i].state.compare_exchange_weak(old_state, ASTBankState::Stage2_Locked))
				{
					const void *ptr = (const char *)m_main_window + m_bank_size * i;

					m_current_bank = i;

					return ptr;
				}
			}
		}
    }
	void release_bank()
	{
		ASTBankEntry *banks = reinterpret_cast<ASTBankEntry *>(m_sub_window);

		banks[m_current_bank].state.store(ASTBankState::Stage2_Unlocked);

		m_current_bank = -1;
	}
public:
    Stage2Parser(T& chaser, size_t bank_size, int bank_num)
        : m_chaser(chaser), m_bank_size(bank_size)
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
    virtual ~Stage2Parser()
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
	void run()
	{
		start();
		wait();
	}
	void start()
	{
		clock_t start_time = clock();

#if defined(CENTAURUS_BUILD_WINDOWS)
		m_thread = CreateThread(NULL, m_stack_size, Stage2Parser<T>::thread_runner, (LPVOID)this, 0, NULL);
#elif defined(CENTAURUS_BUILD_LINUX)
		pthread_attr_t attr;

		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, m_stack_size);
		pthread_create(&m_thread, &attr, Stage2Parser<T>::thread_runner, static_cast<void *>(this));
#endif

		clock_t end_time = clock();
	}
	void wait()
	{
#if defined(CENTAURUS_BUILD_WINDOWS)
		WaitForSingleObject(m_thread, INFINITE);
#elif defined(CENTAURUS_BUILD_LINUX)
		pthread_join(m_thread, NULL);
#endif
	}
};
}
