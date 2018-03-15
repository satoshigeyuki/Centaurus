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
class SlaveParser
{
	static constexpr size_t m_stack_size = 64 * 1024 * 1024;
    size_t m_bank_size;
    T& m_chaser;
	IPCSlave m_ipc;
	const void *m_input;
private:
#if defined(CENTAURUS_BUILD_WINDOWS)
	static DWORD WINAPI thread_runner(LPVOID param)
#elif defined(CENTAURUS_BUILD_LINUX)
	static void *thread_runner(void *param)
#endif
	{
		SlaveParser<T, Tsv> *instance = reinterpret_cast<SlaveParser<T, Tsv> *>(param);

		while (true)
		{
			std::pair<const void *, int> bank = instance->m_ipc.request_bank();

			
		}

#if defined(CENTAURUS_BUILD_WINDOWS)
		ExitThread(0);
#elif defined(CENTAURUS_BUILD_LINUX)
		return NULL;
#endif
	}
	Tsv parse_subtree(const uint64_t *ast, int position)
	{
        std::vector<Tsv> children;

		int machine_id = MACHINE_ID_FROM_MARKER(ast[position]);
		for (int i = position + 1; i < m_bank_size / 8; i++)
		{
			if (IS_START_MARKER(ast[i]))
			{
                children.push_back(parse_subtree(ast, i));
			}
            else if (IS_END_MARKER(ast[i]))
            {
				m_chaser[machine_id](static_cast<void *>(this), );
            }
		}
	}
public:
    SlaveParser(T& chaser, size_t bank_size)
        : m_chaser(chaser), m_bank_size(bank_size)
    {
    }
    virtual ~SlaveParser()
    {
    }
	void run()
	{
		clock_t start_time = clock();

#if defined(CENTAURUS_BUILD_WINDOWS)
		HANDLE hThread = CreateThread(NULL, m_stack_size, SlaveParser<T, Tsv>::thread_runner, (LPVOID)this, 0, NULL);

		WaitForSingleObject(hThread, INFINITE);
#elif defined(CENTAURUS_BUILD_LINUX)
		pthread_t thread;
		pthread_attr_t attr;

		pthread_attr_init(&attr);

		pthread_attr_setstacksize(&attr, m_stack_size);

		pthread_create(&thread, &attr, SlaveParser<T, Tsv>::thread_runner, static_cast<void *>(this));
#endif

		clock_t end_time = clock();
	}
    void operator()(const uint64_t *ast)
    {
        for (int i = 0; i < m_bank_size / 8; i++)
        {
            if (ast[i] & ((uint64_t)1 << 63))
            {
				parse_subtree(ast, i);
            }
        }
    }
};
}
