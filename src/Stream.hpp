#pragma once

#include <string>
#include <exception>
#include <iostream>
#include <sstream>
#include <cwctype>

namespace Centaurus
{
class StreamException : public std::exception
{
    std::string m_msg;
public:
    StreamException() noexcept
    {
    }
    StreamException(const std::string& msg) noexcept
        : m_msg(msg)
    {
    }
    virtual ~StreamException()
    {
    }
    const char *what() const noexcept
    {
        return m_msg.c_str();
    }
};

class Stream
{
    std::wstring m_str;
    std::wstring::iterator m_cur;
    int m_line, m_pos;
    bool m_newline_flag;
public:
    using Sentry = std::wstring::const_iterator;
    Stream(std::wstring&& str)
        : m_str(str), m_line(1), m_pos(0), m_newline_flag(false)
    {
        m_cur = m_str.begin();
    }
    virtual ~Stream()
    {
    }
    void skip_multiline_comment()
    {
        wchar_t ch = get();
        for (; ch != L'\0'; ch = get())
        {
            if (ch == L'*')
            {
                ch = peek();
                if (ch == L'/')
                {
                    get();
                    return;
                }
            }
        }
        throw unexpected(EOF);
    }
    void skip_oneline_comment()
    {
        for (wchar_t ch = get(); ch != L'\0' && ch != L'\n'; ch = get())
            ;
    }
    wchar_t get()
    {
        if (m_cur == m_str.end())
            return 0;
        else
        {
            wchar_t ch = *(m_cur++);
            if (m_newline_flag)
            {
                m_newline_flag = false;
                m_line++;
                m_pos = 0;
            }
            else
            {
                m_pos++;
            }
            if (ch == L'\r')
            {
                if (m_cur == m_str.end())
                    return L'\n';
                if (*m_cur == L'\n')
                {
                    m_newline_flag = true;
                    m_pos++;
                    m_cur++;
                }
            }
            else if (ch == L'\n' || ch == L'\u0085' || ch == L'\u2028' || ch == L'\u2029')
            {
                m_newline_flag = true;
            }
            return ch;
        }
    }
    wchar_t peek()
    {
        if (m_cur == m_str.end())
            return L'\0';
        return *m_cur;
    }
    wchar_t peek(int n)
    {
        for (int i = 0; i < n - 1; i++)
        {
            if (m_cur + i == m_str.end())
                return L'\0';
        }
        return *(m_cur + (n - 1));
    }
    void discard()
    {
        get();
    }
    void discard(int n)
    {
        for (int i = 0; i < n; i++)
            get();
    }
    wchar_t skip_whitespace()
    {
        wchar_t ch;

        while (true)
        {
            ch = peek();

            if (std::iswspace(ch))
            {
                discard();
            }
            else if (ch == L'/')
            {
                ch = peek(2);
                if (ch == L'*')
                {
                    discard(2);
                    skip_multiline_comment();
                }
                else if (ch == L'/')
                {
                    discard(2);
                    skip_oneline_comment();
                }
                else
                {
                    return L'/';
                }
            }
            else
            {
                break;
            }
        }
        return ch;
    }
    wchar_t after_whitespace()
    {
        skip_whitespace();
        return get();
    }
    Sentry sentry()
    {
        return m_cur;
    }
    std::wstring cut(const Sentry& sentry)
    {
        return std::wstring(static_cast<std::wstring::const_iterator>(sentry), static_cast<std::wstring::const_iterator>(m_cur));
    }
    StreamException unexpected(wchar_t ch)
    {
        std::ostringstream stream;

        stream << "Line " << m_line << ", Pos " << m_pos << ": Unexpected character " << std::hex << (int)ch << std::dec;

        return StreamException(stream.str());
    }
    StreamException toomany(int count)
    {
        std::ostringstream stream;

        stream << "Line " << m_line << ", Pos " << m_pos << ": The number of rules exceeded " << std::dec << count;

        return StreamException(stream.str());
    }
};
}
