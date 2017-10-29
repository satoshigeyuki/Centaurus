#pragma once

namespace Centaurus
{
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
    virtual ~Stream
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
            else if (ch == L'\n')
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
        return *(m_cur + 1);
    }
    wchar_t peek(int n)
    {
        for (int i = 0; i < n; i++)
        {
            if (m_cur + i == m_str.end())
                return L'\0';
        }
        return *(m_cur + i);
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
        for (ch = L' '; ; ch = peek())
        {
            switch (ch)
            {
            case L' ':
            case L'\t':
            case L'\r':
            case L'\n':
                discard();
                break;
            case L'/':
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
                break;
            default:
                return ch;
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
        return std::wstring(sentry, m_cur);
    }
    std::exception unexpected(wchar_t ch)
    {
        std::ostringstream stream;

        stream << "Line " << m_line << ", Pos " << m_pos << ": Unexpected character " << stream.narrow(ch, '@');

        std::string msg = stream.str();

        return std::exception(msg.c_str());
    }
};
}
