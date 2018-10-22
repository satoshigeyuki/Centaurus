#include "Util.hpp"

#include <fstream>
#include <wctype.h>

namespace Centaurus
{
std::wostream& operator<<(std::wostream& os, const ATNPath& path)
{
	if (!path.empty())
	{
		for (unsigned int i = 0; i < path.size() - 1; i++)
		{
			os << path[i].first << L'.' << path[i].second << L'/';
		}
		os << path.back().first << L'.' << path.back().second;
	}
	return os;
}
std::wostream& operator<<(std::wostream& os, const IndexVector& v)
{
    os << L'[';
    for (unsigned int i = 0; i < v.size() - 1; i++)
    {
        os << v[i] << L',';
    }
    if (!v.empty())
        os << v.back();
    os << L']';
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
void ATNPath::parse(Stream& stream)
{
    while (true)
    {
        Identifier id(stream);

        wchar_t ch = stream.get();
        if (ch != L'.')
            throw stream.unexpected(ch);

        std::wstring index_str;
        ch = stream.get();
        if (!iswdigit(ch))
            throw stream.unexpected(ch);

        index_str.push_back(ch);

        for (ch = stream.peek(); iswdigit(ch); ch = stream.peek())
        {
            index_str.push_back(ch);
            stream.discard();
        }

        int index = std::stoi(index_str);

        ch = stream.get();
        if (ch == L'\0')
            break;
        else if (ch != L'/')
            throw stream.unexpected(ch);

        emplace_back(id, index);
    }
}
}
