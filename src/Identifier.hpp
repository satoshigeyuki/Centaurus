#pragma once

#include <string>
#include <cwctype>
#include <locale>
#include <codecvt>

#include "Stream.hpp"

namespace Centaurus
{
class Identifier
{
    std::wstring m_id;
public:
    static bool is_symbol_leader(wchar_t ch)
    {
        return ch == L'_' || std::iswalpha(ch);
    }
    static bool is_symbol_char(wchar_t ch)
    {
        return ch == L'_' || std::iswalnum(ch);
    }
    void parse(Stream& stream)
    {
        Stream::Sentry sentry = stream.sentry();

        wchar_t ch = stream.get();
        if (!is_symbol_leader(ch))
        {
            throw stream.unexpected(ch);
        }

        ch = stream.peek();
        for (; is_symbol_char(ch); ch = stream.peek())
        {
            stream.discard();
        }

        m_id = stream.cut(sentry);
    }
    Identifier(Stream& stream)
    {
        parse(stream);
    }
    Identifier(const Identifier& id)
        : m_id (id.m_id)
    {
    }
    Identifier()
    {
    }
    Identifier(const std::wstring& str)
        : m_id (str)
    {
    }
    Identifier(const wchar_t *str)
        : m_id (str)
    {
    }
    const std::wstring& str() const
    {
        return m_id;
    }
    virtual ~Identifier()
    {
    }
    std::string narrow() const
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;

        return converter.to_bytes(m_id);
    }
    bool operator==(const Identifier& id) const
    {
        return m_id == id.m_id;
    }
    operator bool() const
    {
        return !m_id.empty();
    }
};
}

namespace std
{
template<> struct hash<Centaurus::Identifier>
{
    hash<wstring> hasher;
    size_t operator()(const Centaurus::Identifier& id) const
    {
        return hasher(id.str());
    }
};
template<> struct equal_to<Centaurus::Identifier>
{
    bool operator()(const Centaurus::Identifier& x, const Centaurus::Identifier& y) const
    {
        return x.str() == y.str();
    }
};
}
