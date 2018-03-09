#pragma once

#include "IPCSlave.hpp"

namespace Centaurus
{
template<typename TCHAR>
class SlaveParser
{
    size_t m_bank_size;
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

            }
            else
            {
            }
        }
    }
};
}
