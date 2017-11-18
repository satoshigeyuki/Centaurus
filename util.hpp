#pragma once

#include <iostream>
#include <algorithm>
#include <vector>
#include <utility>

#include "identifier.hpp"

namespace Centaurus
{
template<typename TCHAR> TCHAR wide_to_target(char16_t ch)
{
    return static_cast<TCHAR>(ch);
}
class ATNPath
{
    friend std::ostream& operator<<(std::ostream& os, const ATNPath& path);

    std::vector<std::pair<Identifier, int> > m_path;
public:
    ATNPath()
    {
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
    int leaf_id() const
    {
        return m_path.back().first;
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
    /*!
     * @brief Perform destructive sort and copy removing duplicate values
     */
    IndexVector sort_and_unique_copy()
    {
        IndexVector ret;

        std::sort(begin(), end());

        if (!empty())
        {
            ret.push_back(front());

            int prev_value = front();

            for (auto i = begin() + 1; i != end(); i++)
            {
                if (*i != prev_value)
                {
                    ret.push_back(*i);
                    prev_value = *i;
                }
            }
        }

        return ret;
    }
};
std::ostream& operator<<(std::ostream& os, const Identifier& id);
std::ostream& operator<<(std::ostream& os, const ATNPath& path);
std::ostream& operator<<(std::ostream& os, const IndexVector& v);
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
