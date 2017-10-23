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
class UnexpectedException : public std::exception
{
    std::string msg;
public:
    UnexpectedException(wchar_t ch) noexcept
    {
        std::wostringstream stream;

        if (ch == EOF)
            stream << L"Unexpected EOF" << std::endl;
        else
            stream << L"Unexpected character " << ch << std::endl;

        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > converter;

        msg = converter.to_bytes(stream.str());
    }
    virtual ~UnexpectedException()
    {
    }
    const char *what() const noexcept
    {
        return msg.c_str();
    }
};

template<typename TCHAR> class REPattern
{
    DFA dfa;
private:
    DFACharClass<TCHAR> parse_char_class(std::wistringstream& input)
    {
        DFACharClass<TCHAR> cc;

        std::wistringstream::int_type ch;
        bool invert_flag = false;

        ch = input.get();
        if (ch == L'^')
        {
            invert_flag = true;
            ch = input.get();
        }
        while (ch != L']')
        {
            if (ch == EOF)
                throw UnexpectedException(ch);
            if (ch == L'\\')
            {
                ch = input.get();
                switch (ch)
                {
                default:
                    throw UnexpectedException(ch);
                }
            }
            else
            {
                std::wistringstream::int_type start = ch;
            }
        }
        return cc;
    }
    void parse(const wchar_t *i)
    {
        switch (*i)
        {
        case L'[':

            break;
        }
    }
public:
    REPattern(const std::wstring& pattern)
    {
        parse(pattern.c_str());
    }
    virtual ~REPattern()
    {
    }
    
};
}
