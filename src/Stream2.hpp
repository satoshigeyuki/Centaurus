#pragma once

#include <string>

namespace Centaurus
{
class WideStream
{
	std::wstring m_str;
	std::wstring::iterator m_cur;
	int m_line, m_pos;
	bool m_newline_flag;
public:
	using Sentry = std::wstring::const_iterator;
	WideStream(std::wstring&& str)
		: m_str(str), m_line(1), m_pos(0), m_newline_flag(false)
	{
		m_cur = m_str.begin();
	}
	virtual ~WideStream()
	{

	}
	void skip_multiline_comment()
	{

	}
};
}