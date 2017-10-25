#pragma once

#include <exception>
#include <sstream>
#include <locale>
#include <codecvt>

#include "dfa.hpp"
#include "exception.hpp"

#define EOF (-1)

namespace Centaur
{
template<typename TCHAR> class REPattern
{
    NFA<TCHAR> m_nfa;
private:
    NFACharClass<TCHAR> parse_char_class(std::wistream& input)
    {
        NFACharClass<TCHAR> cc;

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
        while (ch != L']')
        {
            bool escaped = false;

            if (ch == 0xFFFF)
            {
                throw ReservedCharException();
            }

            if (ch == EOF)
                throw UnexpectedException(ch);
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
    NFACharClass<TCHAR> parse_single_char(std::wistream::int_type ch)
    {
        return NFACharClass<TCHAR>(ch, ch + 1);
    }
    NFA<TCHAR> parse_sub(std::wistream& stream)
    {
        NFA<TCHAR> new_nfa;
    
        return new_nfa;
    }
    NFA<TCHAR> parse(std::wistream& stream, std::wistream::int_type ch)
    {
        NFA<TCHAR> new_nfa;

        int current_state = -1;

        for (; ch != EOF && ch != L')'; ch = stream.get())
        {
            switch (ch)
            {
            case L'\\':
                ch = stream.get();
                if (ch == EOF)
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
                    current_state = new_nfa.add_state(parse_single_char(ch));
                    break;
                default:
                    throw UnexpectedException(ch);
                }
                break;
            case L'[':
                current_state = new_nfa.add_state(parse_char_class(stream));
                break;
            case L'.':
                current_state = new_nfa.add_state(NFACharClass<TCHAR>::make_star());
                break;
            case L'+':
                //Throw if there is nothing to apply the operator to
                if (current_state == -1)
                    throw UnexpectedException(L'+');
                //Add an epsilon transition to the last state
                new_nfa.add_transition_to(NFACharClass<TCHAR>(), current_state);
                current_state = -1;
                break;
            case L'*':
                //Throw if there is nothing to apply the operator to
                if (current_state == -1)
                    throw UnexpectedException(L'*');
                //Add an epsilon transition to the last state
                new_nfa.add_transition_to(NFACharClass<TCHAR>(), current_state);
                //Add a confluence
                new_nfa.add_state(NFACharClass<TCHAR>());
                //Add a skipping transition
                new_nfa.add_transition_from(NFACharClass<TCHAR>(), current_state);
                current_state = -1;
                break;
            case L'?':
                if (current_state == -1)
                    throw UnexpectedException(L'?');
                //Add a skipping transition
                new_nfa.add_transition_from(NFACharClass<TCHAR>(), current_state);
                current_state = -1;
                break;
            case L'(':
                current_state = new_nfa.concat(parse(stream, stream.get()));
                break;
            case L'{':
                if (current_state == -1)
                    throw UnexpectedException(L'{');

            case L'|':
                if (current_state == -1)
                    throw UnexpectedException(L'+');
                ch = stream.get();
                switch (ch)
                {
                case EOF:
                case L'+':
                case L'?':
                case L'{':
                case L'
                    throw UnexpectedException(ch);
                }
                break;
            default:
                current_state = new_nfa.add_state(parse_single_char(ch));
                break;
            }
        }

        return new_nfa;
    }
public:
    REPattern(const std::wstring& pattern)
    {
        std::wistringstream stream(pattern);

        std::wistringstream::int_type ch = stream.get();

        m_nfa = parse(stream, ch);
    }
    virtual ~REPattern()
    {
    }
};
}
