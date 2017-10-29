#pragma once

#include <iostream>
#include <string>
#include <unordered_map>

#include "identifier.hpp"
#include "atn.hpp"

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
private:
    void parse_literal_string(Stream& stream)
    {
        wchar_t ch = stream.get();
        for (; ch != L'\0'; ch = stream.get())
        {
            if (ch == L'"')
                return;
        }
        throw stream.unexpected(EOF);
    }
    void parse_literal_character(Stream& stream)
    {
        wchar_t ch = stream.get();
        if (ch == '\\')
            ch = stream.get();
        ch = stream.get();
        if (ch != '\'')
            throw stream.unexpected(ch);
    }
    void parse(Stream& stream)
    {
        while (true)
        {
            wchar_t ch = stream.get_next_char();

            switch (ch)
            {
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
    CppAlienCode(Stream& stream)
    {
        Stream::Sentry begin = stream.sentry();

        parse(stream);

        m_str = stream.cut(begin);
    }
    virtual ~CppAlienCode()
    {
    }
};
/* class Production
{
    Identifier m_lhs;

private:
    void parse(Stream& stream)
    {
        m_lhs = Identifier(stream);

        wchar_t sep = stream.after_whitespace();

        if (sep != L':')
            stream.unexpected(sep);

        wchar_t ch;

        for (ch = skip_whitespace(); ;)
        {
            if (
        }
    }
public:
    Production(Stream& stream)
    {
        parse(stream);
    }
    virtual ~Production()
    {
    }
};*/
template<TCHAR> class Grammar
{
    std::unordered_map<Identifier, ATN<TCHAR> > m_networks;
private:
    void parse(Stream& stream)
    {
        while (1)
        {
            wchar_t ch = stream.skip_whitespace();

            if (ch == L'\0')
                break;

            m_networks.emplace(Identifier(stream), ATN<TCHAR>(stream));
        }
    }
public:
    Grammar(Stream& stream)
    {
        parse(stream);
    }
    ~Grammar()
    {
    }
};
}
