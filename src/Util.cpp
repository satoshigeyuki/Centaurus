#include "Util.hpp"

#include <fstream>

namespace Centaurus
{
std::ostream& operator<<(std::ostream& os, const Identifier& id)
{
    os << id.narrow();
    return os;
}
std::ostream& operator<<(std::ostream& os, const ATNPath& path)
{
    if (!path.m_path.empty())
    {
        for (unsigned int i = 0; i < path.m_path.size() - 1; i++)
        {
            os << path.m_path[i].first << '.' << path.m_path[i].second << '/';
        }
        os << path.m_path.back().first << '.' << path.m_path.back().second;
    }
    return os;
}
std::wostream& operator<<(std::wostream& os, const ATNPath& path)
{
	if (!path.m_path.empty())
	{
		for (unsigned int i = 0; i < path.m_path.size() - 1; i++)
		{
			os << path.m_path[i].first << L'.' << path.m_path[i].second << L'/';
		}
		os << path.m_path.back().first << L'.' << path.m_path.back().second;
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
std::string readmbsfromfile(const char *filename)
{
	std::ifstream ifs(filename);

	return std::string(std::istreambuf_iterator<char>(ifs), {});
}
std::wstring readwcsfromfile(const char *filename)
{
	std::wifstream ifs(filename);

	return std::wstring(std::istreambuf_iterator<wchar_t>(ifs), {});
}
}
