#include "util.hpp"

namespace Centaurus
{
std::ostream& operator<<(std::ostream& os, const ATNPath& path)
{
    for (unsigned int i = 0; i < path.m_path.size() - 1; i++)
    {
        os << path.m_path[i].first << "." << path.m_path[i].second << "/";
    }
    if (!path.m_path.empty())
    {
        os << path.m_path.back().first << "." << path.m_path.back().second;
    }
    return os;
}
std::ostream& operator<<(std::ostream& os, const IndexVector& v)
{
    os << '[';
    for (unsigned int i = 0; i < v.size() - 1; i++)
    {
        os << v[i] << ',';
    }
    if (!v.empty())
        os << v.back();
    os << ']';
    return os;
}
}
