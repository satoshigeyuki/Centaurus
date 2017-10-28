#pragma once

#include <iostream>
#include <list>

namespace Centaurus
{
class AlienCode
{
    std::wstring m_str;
public:
    AlienCode()
    {
    }
    virtual ~AlienCode()
    {
    }
};
class CppAlienCode : public AlienCode
{
    using AlienCode::m_str;

    void parse_multiline_comment(WideStream& stream)
    {
        wchar_t ch = stream.get();
        for (; ch != L'\0'; ch = stream.get())
        {
            if (ch == L'*')
            {
                ch = stream.get();
                if (ch == L'/')
                {
                    return;
                }
            }
        }
        throw stream.unexpected(EOF);
    }
    void parse_oneline_comment(WideStream& stream)
    {
        wchar_t ch = stream.get();
        for (; ch != L'\0'; ch = stream.get())
        {
            if (ch == L'\n')
                return;
            if (ch == L'\r')
            {
                ch = stream.get();
                if (ch == L'\n')
                    return;
            }
        }
        throw stream.unexpected(EOF);
    }
    void parse_literal_string(WideStream& stream)
    {
        wchar_t ch = stream.get();
        for (; ch != L'\0'; ch = stream.get())
        {
            if (ch == L'"')
                return;
        }
        throw stream.unexpected(EOF);
    }
    void parse_literal_character(WideStream& stream)
    {
        wchar_t ch = stream.get();
        if (ch == '\\')
            ch = stream.get();
        ch = stream.get();
        if (ch != '\'')
            throw stream.unexpected(ch);
    }
    void parse(WideStream& stream)
    {
        while (true)
        {
            wchar_t ch = stream.skip_whitespace();

            switch (ch)
            {
            case L'/':
                ch = stream.get();
                if (ch == L'/')
                    parse_oneline_comment(stream);
                else if (ch == L'*')
                    parse_multiline_comment(stream);
                break;
            case L'"':
                parse_literal_string(stream);
                break;
            case L'\'':
                parse_literal_character(stream);
                break;
            }
        }
    }
public:
    CppAlienCode(WideStream& stream)
    {
        WideStream::Sentry begin = stream.sentry();

        parse(stream);

        m_str = stream.cut(begin);
    }
    virtual ~CppAlienCode()
    {
    }
};
class Production
{
};
class Grammar
{
    std::list<Production> m_productions;
public:
    Grammar(WideStream& stream)
    {

    }
    ~Grammar()
    {
    }
};
}
