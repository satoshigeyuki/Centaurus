#pragma once

#include <iostream>
#include <algorithm>
#include <vector>
#include <utility>

#include "Identifier.hpp"

namespace Centaurus
{
static char ASCII(char ch)
{
	return ch;
}
static char ASCII(wchar_t ch)
{
	switch (ch)
	{
	case L'\0': return '\0';
	case L'\r': return '\r';
	case L'\n': return '\n';
	case L'/': return '/';
	case L'*': return '*';
	case L'\\': return '\\';
	case L' ': return ' ';
	case L'[': return '[';
	case L']': return ']';
	case L'+': return '+';
	case L'-': return '-';
	case L'"': return '"';
	case L'\'': return '\'';
	case L':': return ':';
	case L';': return ';';
	case L'|': return '|';
	case L'(': return '(';
	case L')': return ')';
	case L'?': return '?';
	case L'.': return '.';
	case L'{': return '{';
	case L'}': return '}';
	case L',': return ',';
	default: return '@';
	}
}
static char ASCII(char16_t ch)
{
	switch (ch)
	{
	case u'\0': return '\0';
	case u'\r': return '\r';
	case u'\n': return '\n';
	case u'/': return '/';
	case u'*': return '*';
	case u'\\': return '\\';
	case u' ': return ' ';
	case u'[': return '[';
	case u']': return ']';
	case u'+': return '+';
	case u'-': return '-';
	case u'"': return '"';
	case u'\'': return '\'';
	case u':': return ':';
	case u';': return ';';
	case u'|': return '|';
	case u'(': return '(';
	case u')': return ')';
	case u'?': return '?';
	case u'.': return '.';
	case u'{': return '{';
	case u'}': return '}';
	case u',': return ',';
	default: return '@';
	}
}
static char ASCII(char32_t ch)
{
	switch (ch)
	{
	case U'\0': return '\0';
	case U'\r': return '\r';
	case U'\n': return '\n';
	case U'/': return '/';
	case U'*': return '*';
	case U'\\': return '\\';
	case U' ': return ' ';
	case U'[': return '[';
	case U']': return ']';
	case U'+': return '+';
	case U'-': return '-';
	case U'"': return '"';
	case U'\'': return '\'';
	case U':': return ':';
	case U';': return ';';
	case U'|': return '|';
	case U'(': return '(';
	case U')': return ')';
	case U'?': return '?';
	case U'.': return '.';
	case U'{': return '{';
	case U'}': return '}';
	case U',': return ',';
	default: return '@';
	}
}
template<typename TCHAR>
static TCHAR XCHAR(char ch)
{
	return static_cast<TCHAR>(ch);
}
template<> static wchar_t XCHAR<wchar_t>(char ch)
{
	switch (ch)
	{
	case '\0': return L'\0';
	case '\n': return L'\n';
	default:
		return static_cast<wchar_t>(ch);
	}
}
template<> static char16_t XCHAR<char16_t>(char ch)
{
	switch (ch)
	{
	case '\0': return u'\0';
	case '\n': return u'\n';
	default:
		return static_cast<char16_t>(ch);
	}
}
template<> static char32_t XCHAR<char32_t>(char ch)
{
	switch (ch)
	{
	case '\0': return U'\0';
	case '\n': return U'\n';
	default:
		return static_cast<char32_t>(ch);
	}
}
template<typename TCHAR> TCHAR wide_to_target(char16_t ch)
{
    return static_cast<TCHAR>(ch);
}
template<typename TCHAR> int target_strlen(const TCHAR *str)
{
    int i;
    for (i = 0; str[i] != 0; i++)
        ;
    return i;
}
class ATNPath
{
    friend std::ostream& operator<<(std::ostream& os, const ATNPath& path);

    std::vector<std::pair<Identifier, int> > m_path;
public:
    ATNPath()
    {
    }
    ATNPath(const Identifier& id, int index)
    {
        m_path.emplace_back(id, index);
    }
    ATNPath(const ATNPath& path)
        : m_path(path.m_path)
    {
    }
    virtual ~ATNPath()
    {
    }
    void push(const Identifier& id, int index)
    {
        m_path.emplace_back(id, index);
    }
    void push(const std::pair<Identifier, int>& p)
    {
        m_path.push_back(p);
    }
    void pop()
    {
        m_path.pop_back();
    }
    int count(const Identifier& id, int index) const
    {
        return std::count(m_path.cbegin(), m_path.cend(), std::pair<Identifier, int>(id, index));
    }
    ATNPath add(const Identifier& id, int index) const
    {
        ATNPath new_path(*this);

        new_path.push(id, index);

        return new_path;
    }
    ATNPath parent_path() const
    {
        ATNPath new_path(*this);

        new_path.pop();

        return new_path;
    }
    bool operator==(const ATNPath& path) const
    {
        return std::equal(m_path.cbegin(), m_path.cend(), path.m_path.cbegin());
    }
    size_t hash() const
    {
        size_t ret = 0;
        std::hash<Identifier> hasher;
        for (const auto& p : m_path)
        {
            ret += hasher(p.first) + static_cast<size_t>(p.second);
        }
        return ret;
    }
    const std::pair<Identifier, int>& leaf() const
    {
        return m_path.back();
    }
    int leaf_index() const
    {
        return m_path.back().second;
    }
    const Identifier& leaf_id() const
    {
        return m_path.back().first;
    }
    int depth() const
    {
        return m_path.size();
    }
    ATNPath replace_index(int index) const
    {
        ATNPath path(*this);

        path.m_path.back().second = index;

        return path;
    }
    int compare(const ATNPath& p) const
    {
        if (depth() < p.depth())
        {
            return -1;
        }
        else if (depth() > p.depth())
        {
            return +1;
        }
        else
        {
            for (int i = 0; i < depth(); i++)
            {
                int id_cmp = m_path[i].first.str().compare(p.m_path[i].first.str());

                if (id_cmp != 0) return id_cmp;

                if (m_path[i].second < p.m_path[i].second)
                    return -1;
                else if (m_path[i].second > p.m_path[i].second)
                    return +1;
            }
        }
        return 0;
    }
    bool find(const Identifier& id, int index) const
    {
        return std::find_if(m_path.cbegin(), m_path.cend(), [&](const std::pair<Identifier, int>& p) -> bool
        {
            return p.first == id && p.second == index;
        }) != m_path.cend();
    }
};
class IndexVector : public std::vector<int>
{
public:
    IndexVector()
    {
    }
    explicit IndexVector(size_type n)
        : std::vector<int>(n)
    {
    }
    IndexVector(size_type n, bool reserve_flag = true)
        : std::vector<int>(reserve_flag ? 0 : n)
    {
        if (reserve_flag)
            reserve(n);
    }
    template<typename InputIterator>
    IndexVector(InputIterator a, InputIterator b)
        : std::vector<int>(a, b)
    {
    }
    virtual ~IndexVector()
    {
    }
    IndexVector& operator+=(const IndexVector& v)
    {
        reserve(size() + v.size());
        insert(end(), v.cbegin(), v.cend());
        return *this;
    }
    void sort()
    {
        std::sort(begin(), end());
    }
	bool includes(int i) const
	{
		return std::find(cbegin(), cend(), i) != cend();
	}
};
std::ostream& operator<<(std::ostream& os, const Identifier& id);
std::ostream& operator<<(std::ostream& os, const ATNPath& path);
std::ostream& operator<<(std::ostream& os, const IndexVector& v);

using ATNStateStack = ATNPath;
}

namespace std
{
template<> struct hash<Centaurus::ATNPath>
{
    size_t operator()(const Centaurus::ATNPath& path) const
    {
        return path.hash();
    }
private:
};
template<> struct equal_to<Centaurus::ATNPath>
{
    bool operator()(const Centaurus::ATNPath& x, const Centaurus::ATNPath& y) const
    {
        return x == y;
    }
};
}
