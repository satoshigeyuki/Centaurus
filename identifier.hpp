#pragma once

#include <string>
#include <cwctype>

#include "stream.hpp"

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
        Stream::Sentry sentry = stream.get_sentry();

        wchar_t ch = stream.get();
        if (!is_symbol_leader(ch))
            throw stream.unexpected(ch);

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
    const std::wstring& str() const
    {
        return m_id;
    }
    virtual ~Identifier()
    {
    }
};
}

namespace std
{
template<> class hash<Identifier>
{
    hash<wstring> hasher;
public:
    size_t operator()(const Idenfifier& id) const
    {
        return hasher(id.str());
    }
};
template<> class equal_to<Identifier>
{
public:
    bool operator()(const Identifier& x, const Identifier& y) const
    {
        return x.str() == y.str();
    }
};
}
