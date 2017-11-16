#include "util.hpp"

namespace Centaurus
{
std::wostream& operator<<(std::wostream& os, const Identifier& id)
{
    os << id.str();
    return os;
}
std::wostream& operator<<(std::wostream& os, const ATNPath& path)
{
    for (unsigned int i = 0; i < path.m_path.size() - 1; i++)
    {
        //os << path.m_path[i].first << L"." << path.m_path[i].second << L"/";
    }
    if (!path.m_path.empty())
    {
        //os << path.m_path.back().first << L"." << path.m_path.back().second;
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
