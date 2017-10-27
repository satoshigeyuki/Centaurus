#pragma once

#include <exception>
#include <sstream>
#include <locale>
#include <codecvt>

#include "nfa.hpp"
#include "dfa.hpp"
#include "exception.hpp"

namespace Centaur
{
template<typename TCHAR> class REPattern
{
    NFA<TCHAR> m_nfa;
    DFA<TCHAR> m_dfa;
private:
    CharClass<TCHAR> parse_char_class(std::wistream& input)
    {
        CharClass<TCHAR> cc;

        std::wistream::int_type ch;
        bool invert_flag = false;

        ch = input.get();
        if (ch == L'^')
        {
            invert_flag = true;
            ch = input.get();
        }

        std::wistream::int_type start = EOF, end = EOF;
        enum
        {
            CC_STATE_START = 0,
            CC_STATE_RANGE,
            CC_STATE_END
        } state = CC_STATE_START;
        for (; ch != L']'; ch = input.get())
        {
            bool escaped = false;

            if (ch == 0xFFFF)
            {
                throw ReservedCharException();
            }

            if (input.fail())
                throw UnexpectedException(EOF);
            if (ch == L'\\')
            {
                ch = input.get();
                switch (ch)
                {
                case L'\\':
                    ch = L'\\';
                    break;
                case L'-':
                    ch = L'-';
                    break;
                default:
                    throw UnexpectedException(ch);
                }
                escaped = true;
            }
            switch (state)
            {
            case CC_STATE_START:
                if (!escaped && ch == L'-')
                    throw UnexpectedException(ch);
                start = ch;
                state = CC_STATE_RANGE;
                break;
            case CC_STATE_RANGE:
                if (!escaped && ch == L'-')
                {
                    state = CC_STATE_END;
                }
                else
                {
                    cc |= Range<TCHAR>(start, start + 1);
                    start = ch;
                    state = CC_STATE_RANGE;
                }
                break;
            case CC_STATE_END:
                if (!escaped && ch == L'-')
                {
                    throw UnexpectedException(ch);
                }
                else
                {
                    cc |= Range<TCHAR>(start, ch + 1);
                    state = CC_STATE_START;
                }
                break;
            }
        }
        if (state == CC_STATE_RANGE)
        {
            cc |= Range<TCHAR>(start, start + 1);
        }
        else if (state == CC_STATE_END)
        {
            throw UnexpectedException(L']');
        }
        return invert_flag ? ~cc : cc;
    }
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
    NFA<TCHAR> parse_unit(std::wistream& stream, std::wistream::int_type ch)
    {
        NFA<TCHAR> new_nfa;

        switch (ch)
        {
        case L'\\':
            ch = stream.get();
            if (stream.fail())
                throw UnexpectedException(EOF);
            switch (ch)
            {
            case L'[':
            case L']':
            case L'+':
            case L'*':
            case L'{':
            case L'}':
            case L'?':
            case L'\\':
            case L'|':
            case L'(':
            case L')':
            case L'.':
                new_nfa.add_state(parse_single_char(ch));
                break;
            default:
                throw UnexpectedException(ch);
            }
            break;
        case L'[':
            new_nfa.add_state(parse_char_class(stream));
            break;
        case L'.':
            new_nfa.add_state(CharClass<TCHAR>::make_star());
            break;
        case L'(':
            new_nfa.concat(parse(stream));
            break;
        default:
            new_nfa.add_state(parse_single_char(ch));
            break;
        }
        ch = stream.get();
        switch (ch)
        {
        case L'+':
            //Add an epsilon transition to the last state
            new_nfa.add_transition_to(CharClass<TCHAR>(), 1);
            break;
        case L'*':
            //Add an epsilon transition to the last state
            new_nfa.add_transition_to(CharClass<TCHAR>(), 1);
            //Add a confluence
            new_nfa.add_state(CharClass<TCHAR>());
            //Add a skipping transition
            new_nfa.add_transition_from(CharClass<TCHAR>(), 1);
            break;
        case L'?':
            //Add a skipping transition
            new_nfa.add_transition_from(CharClass<TCHAR>(), 1);
            break;
        default:
            stream.unget();
            break;
        }

        return new_nfa;
    }
    NFA<TCHAR> parse_selection(std::wistream& stream, std::wistream::int_type ch)
    {
        NFA<TCHAR> new_nfa = parse_unit(stream, ch);

        for (ch = stream.get(); ; ch = stream.get())
        {
            if (ch != L'|')
            {
                stream.unget();
                break;
            }
            new_nfa.select(parse_unit(stream, stream.get()));
        }

        return new_nfa;
    }
    NFA<TCHAR> parse(std::wistream& stream)
    {
        NFA<TCHAR> new_nfa;

        for (std::wistream::int_type ch = stream.get(); ch != L')' && stream.good(); ch = stream.get())
        {
            new_nfa.concat(parse_selection(stream, ch));
        }

        return new_nfa;
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
