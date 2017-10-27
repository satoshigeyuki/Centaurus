#pragma once

#include <algorithm>

namespace Centaur
{
class IndexVector : std::vector<int>
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
    virtual ~IndexVector()
    {
    }
    IndexVector& operator+=(const IndexVector& v)
    {
        reserve(size() + v.size());
        insert(end(), v.cbegin(), v.cend());
        return *this;
    }
    IndexVector sort_and_unique_copy()
    {
        IndexVector ret;

        std::sort(begin(), end());

        std::unique_copy(begin(), end(), ret.end());

        return ret;
    }
};
}
