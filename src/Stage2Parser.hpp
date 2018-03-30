#pragma once

#if defined(CENTAURUS_BUILD_WINDOWS)
#include <Windows.h>
#elif defined(CENTAURUS_BUILD_LINUX)
#include <unistd.h>
#endif

#include <stdint.h>
#include <time.h>
#include "IPCSlave.hpp"
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
template<class T, typename Tsv>
class Stage2Parser
{
	static constexpr size_t m_stack_size = 64 * 1024 * 1024;
    size_t m_bank_size;
    T& m_chaser;
	IPCSlave m_ipc;
	const void *m_input;
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
		Stage2Parser<T, Tsv> *instance = reinterpret_cast<Stage2Parser<T, Tsv> *>(param);

		while (true)
		{
			std::pair<const void *, int> bank = instance->m_ipc.request_bank();

			reduce_bank(reinterpret_cast<uint64_t *>(bank.first));
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
		std::vector<void *> children;
		for (i = position + 1; i < m_bank_size / 8; i++)
		{
			CSTMarker marker(ast[i]);
			if (marker.is_start_marker())
			{
                i = parse_subtree(ast, i);
			}
            else if (marker.is_end_marker())
            {
				//Invoke chaser to derive the semantic value for this symbol
				return i + 1;
            }
		}
		return i;
	}
public:
    Stage2Parser(T& chaser, size_t bank_size)
        : m_chaser(chaser), m_bank_size(bank_size)
    {
    }
    virtual ~Stage2Parser()
    {
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
		m_thread = CreateThread(NULL, m_stack_size, Stage2Parser<T, Tsv>::thread_runner, (LPVOID)this, 0, NULL);
#elif defined(CENTAURUS_BUILD_LINUX)
		pthread_attr_t attr;

		pthread_attr_init(&attr);

		pthread_attr_setstacksize(&attr, m_stack_size);

		pthread_create(&m_thread, &attr, Stage2Parser<T, Tsv>::thread_runner, static_cast<void *>(this));
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
