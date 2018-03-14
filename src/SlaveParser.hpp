#pragma once

#if defined(CENTAURUS_BUILD_WINDOWS)
#include <Windows.h>
#elif defined(CENTAURUS_BUILD_LINUX)
#include <unistd.h>
#endif

#include "IPCSlave.hpp"
#include "CodeGenEM64T.hpp"

#define MACHINE_ID_FROM_MARKER(m) (((m) >> 48) & 0x7FFF)
#define OFFSET_FROM_MARKER(m) ((m) & (((uint64_t)1 << 48) - 1))
#define IS_START_MARKER(m) ((m) & ((uint64_t)1 << 63))
#define IS_END_MARKER(m) (!((m) & ((uint64_t)1 << 63)))

namespace Centaurus
{
template<typename TCHAR, typename TSV>
class SlaveParser
{
    size_t m_bank_size;
    ChaserEM64T<TCHAR> m_chaser;
	IPCSlave m_ipc;
private:
#if defined(CENTAURUS_BUILD_WINDOWS)
	static DWORD WINAPI thread_runner(LPVOID param)
	{
		SlaveParser<TCHAR, TSV> *instance = reinterpret_cast<SlaveParser<TCHAR, TSV> *>(param);

		ExitThread(0);
	}
#elif defined(CENTAURUS_BUILD_LINUX)
	static void *thread_runner(void *param)
	{
		SlaveParser<TCHAR, TSV> *instance = reinterpret_cast<SlaveParser<TCHAR, TSV> *>(param);
	}
#endif
	TSV parse_subtree(const uint64_t *ast, int position)
	{
        std::vector<TSV> children;

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
    SlaveParser(const Grammar<TCHAR>& grammar, size_t bank_size)
        : m_chaser(grammar), m_bank_size(bank_size)
    {
    }
    virtual ~SlaveParser()
    {
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
	void run()
	{
		
	}
};
}
