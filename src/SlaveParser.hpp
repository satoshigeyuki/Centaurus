#pragma once

#if defined(CENTAURUS_BUILD_WINDOWS)
#include <Windows.h>
#elif defined(CENTAURUS_BUILD_LINUX)
#include <unistd.h>
#endif

#include "IPCSlave.hpp"

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
private:
	TSV *parse_subtree(const uint64_t *ast, int position)
	{
		int machine_id = MACHINE_ID_FROM_MARKER(ast[position]);
		for (int i = position + 1; i < m_bank_size / 8; i++)
		{
			if (IS_START_MARKER(ast[i]))
			{
				parse_subtree(ast, i);
			}
		}
	}
public:
    SlaveParser(size_t bank_size)
        : m_bank_size(bank_size)
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
};
}
