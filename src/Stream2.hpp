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
	wchar_t get()
	{
		if (m_cur == m_str.end())
		{
			return L'\0';
		}
		else
		{
			wchar_t ch = *(m_cur++);
			if (m_newline_flag)
			{
				m_newline_flag = false;
				m_line++;
				m_pos = 0;
			}
			else
			{
				m_pos++;
			}
			if (ch == L'\r')
			{
				if (m_cur == m_str.end())
				{
					return L'\n';
				}
				if (*m_cur == L'\n')
				{
					m_newline_flag = true;
					m_pos++;
					m_cur++;
				}
			}
			else if (ch == L'\n' || ch == L'\u0085' || ch == L'\u2028' || ch == L'\u2029')
			{
				m_newline_flag = true;
			}
			return ch;
		}
	}
	wchar_t peek()
	{
		if (m_cur == m_str.end())
			return L'\0';
		return *m_cur;
	}
	wchar_t peek(int n)
	{

	}
};
}