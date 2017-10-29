#pragma once

#include <exception>
#include <sstream>
#include <locale>
#include <codecvt>

#include "nfa.hpp"
#include "dfa.hpp"
#include "exception.hpp"

namespace Centaurus
{
template<typename TCHAR> class REPattern
{
    NFA<TCHAR> m_nfa;
    DFA<TCHAR> m_dfa;
private:
    std::pair<int, int> parse_repetition_count(std::wistream& stream)
    {
        int min = 0, max = 0;

        std::wistream::int_type ch = stream.get();

        for (; L'0' <= ch && ch <= L'9'; ch = stream.get())
        {
            min = min * 10 + static_cast<int>(ch - L'0');
        }
        if (ch == L',')
        {
            ch = stream.get();
            for (; L'0' <= ch && ch <= L'9'; ch = stream.get())
            {
                max = max * 10 + static_cast<int>(ch - L'0');
            }
            if (ch == L'}')
            {
                return std::pair<int, int>(min, max);
            }
            else
            {
                throw UnexpectedException(ch);
            }
        }
        else if (ch == L'}')
        {
            return std::pair<int, int>(min, min);
        }
        else
        {
            throw UnexpectedException(ch);
        }
    }
    CharClass<TCHAR> parse_single_char(std::wistream::int_type ch)
    {
        return CharClass<TCHAR>(ch, ch + 1);
    }
public:
    REPattern(const std::wstring& pattern)
    {
        std::wistringstream stream(pattern);

        m_nfa = parse(stream);

        m_dfa = DFA<TCHAR>(m_nfa);
    }
    virtual ~REPattern()
    {
    }
    void print_dfa(std::ostream& os)
    {
        m_dfa.print(os, "DFA");
    }
    void print_nfa(std::ostream& os)
    {
        m_nfa.print(os, "NFA");
    }
};
}
