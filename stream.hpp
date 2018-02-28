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
    std::u16string m_str;
    std::u16string::iterator m_cur;
    int m_line, m_pos;
    bool m_newline_flag;
public:
    using Sentry = std::u16string::const_iterator;
    Stream(std::u16string&& str)
        : m_str(str), m_line(1), m_pos(0), m_newline_flag(false)
    {
        m_cur = m_str.begin();
    }
    virtual ~Stream()
    {
    }
    void skip_multiline_comment()
    {
        char16_t ch = get();
        for (; ch != u'\0'; ch = get())
        {
            if (ch == u'*')
            {
                ch = peek();
                if (ch == u'/')
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
        for (char16_t ch = get(); ch != u'\0' && ch != u'\n'; ch = get())
            ;
    }
    char16_t get()
    {
        if (m_cur == m_str.end())
            return 0;
        else
        {
            char16_t ch = *(m_cur++);
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
            if (ch == u'\r')
            {
                if (m_cur == m_str.end())
                    return u'\n';
                if (*m_cur == u'\n')
                {
                    m_newline_flag = true;
                    m_pos++;
                    m_cur++;
                }
            }
            else if (ch == u'\n' || ch == u'\u0085' || ch == u'\u2028' || ch == u'\u2029')
            {
                m_newline_flag = true;
            }
            return ch;
        }
    }
    char16_t peek()
    {
        if (m_cur == m_str.end())
            return u'\0';
        return *m_cur;
    }
    char16_t peek(int n)
    {
        for (int i = 0; i < n - 1; i++)
        {
            if (m_cur + i == m_str.end())
                return u'\0';
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
    char16_t skip_whitespace()
    {
        char16_t ch;

        while (true)
        {
            ch = peek();

            if (std::iswspace(ch))
            {
                discard();
            }
            else if (ch == u'/')
            {
                ch = peek(2);
                if (ch == u'*')
                {
                    discard(2);
                    skip_multiline_comment();
                }
                else if (ch == u'/')
                {
                    discard(2);
                    skip_oneline_comment();
                }
                else
                {
                    return u'/';
                }
            }
            else
            {
                break;
            }
        }
        return ch;
    }
    char16_t after_whitespace()
    {
        skip_whitespace();
        return get();
    }
    Sentry sentry()
    {
        return m_cur;
    }
    std::u16string cut(const Sentry& sentry)
    {
        return std::u16string(static_cast<std::u16string::const_iterator>(sentry), static_cast<std::u16string::const_iterator>(m_cur));
    }
    StreamException unexpected(char16_t ch)
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
