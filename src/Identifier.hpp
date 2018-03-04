#pragma once

#include <string>
#include <cwctype>
#include <locale>
#include <codecvt>

#include "stream.hpp"

namespace Centaurus
{
class Identifier
{
    std::u16string m_id;
public:
    static bool is_symbol_leader(char16_t ch)
    {
        return ch == u'_' || std::iswalpha(ch);
    }
    static bool is_symbol_char(char16_t ch)
    {
        return ch == u'_' || std::iswalnum(ch);
    }
    void parse(Stream& stream)
    {
        Stream::Sentry sentry = stream.sentry();

        char16_t ch = stream.get();
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
    Identifier(const std::u16string& str)
        : m_id (str)
    {
    }
    Identifier(const char16_t *str)
        : m_id (str)
    {
    }
    const std::u16string& str() const
    {
        return m_id;
    }
    virtual ~Identifier()
    {
    }
    std::string narrow() const
    {
        std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> converter;

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
    hash<u16string> hasher;
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
