#pragma once

#include <string>
#include <exception>
#include <sstream>
#include "Charset.hpp"

namespace Centaurus
{
class StreamException : public std::exception
{
	std::string m_msg;
public:
	StreamException() noexcept
	{
	}
	StreamException(const std::string& msg) noexcept
		: m_msg(msg)
	{
	}
	virtual ~StreamException()
	{
	}
	const char *what() const noexcept
	{
		return m_msg.c_str();
	}
};
template<typename TCHAR>
class GenericStream
{
public:
	virtual ~GenericStream()
	{

	}
};
template<typename TCHAR>
class MemoryStream : public GenericStream<TCHAR>
{
	std::basic_string<TCHAR> m_str;
	typename std::basic_string<TCHAR>::iterator m_cur;
	int m_line, m_pos;
	bool m_newline_flag;
public:
	using Sentry = typename std::basic_string<TCHAR>::const_iterator;
	MemoryStream(std::basic_string<TCHAR>&& str)
		: m_str(str), m_line(1), m_pos(0), m_newline_flag(false)
	{
		m_cur = m_str.begin();
	}
	MemoryStream(const std::basic_string<TCHAR>& str)
		: m_str(str), m_line(1), m_pos(0), m_newline_flag(false)
	{
		m_cur = m_str.begin();
	}
	virtual ~MemoryStream()
	{

	}
	void skip_multiline_comment()
	{
		TCHAR ch = get();
		for (; ASCII(ch) != '\0'; ch = get())
		{
			if (ASCII(ch) == '*')
			{
				ch = peek();
				if (ASCII(ch) == '/')
				{
					get();
					return;
				}
			}
		}
		throw unexpected(EOF);
	}
	void skip_oneline_comment()
	{
		for (char ch = ASCII(get()); ch != '\0' && ch != '\n'; ch = ASCII(get()))
			;
	}
	TCHAR get()
	{
		if (m_cur == m_str.end())
		{
			return XCHAR<TCHAR>('\0');
		}
		else
		{
			TCHAR ch = *(m_cur++);
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
			if (ASCII(ch) == '\r')
			{
				if (m_cur == m_str.end())
				{
					return XCHAR<TCHAR>('\n');
				}
				else
				{
					if (ASCII(*m_cur) == '\n')
					{
						m_newline_flag = true;
						m_pos++;
						m_cur++;
					}
				}
			}
			else if (ASCII(ch) == '\n')
			{
				m_newline_flag = true;
			}
			return ch;
		}
	}
	TCHAR peek()
	{
		if (m_cur == m_str.end())
			return XCHAR<TCHAR>('\0');
		return *m_cur;
	}
	TCHAR peek(int n)
	{
		for (int i = 0; i < n - 1; i++)
		{
			if (m_cur + i == m_str.end())
				return XCHAR<TCHAR>('\0');
		}
		return *(m_cur + (n - 1));
	}
	void discard()
	{
		get();
	}
	void discard(int n)
	{
		for (int i = 0; i < n; i++)
			get();
	}
	TCHAR skip_whitespace()
	{
		TCHAR ch;

		while (true)
		{
			ch = peek();

			if (ASCII(ch) == ' ' || ASCII(ch) == '\t' || ASCII(ch) == '\n')
			{
				discard();
			}
			else if (ASCII(ch) == '/')
			{
				ch = peek(2);
				if (ASCII(ch) == '*')
				{
					discard(2);
					skip_multiline_comment();
				}
				else if (ASCII(ch) == '/')
				{
					discard(2);
					skip_oneline_comment();
				}
				else
				{
					return XCHAR<TCHAR>('/');
				}
			}
			else
			{
				break;
			}
		}
		return ch;
	}
	TCHAR after_whitespace()
	{
		skip_whitespace();
		return get();
	}
	Sentry sentry()
	{
		return m_cur;
	}
	std::basic_string<TCHAR> cut(const Sentry& sentry)
	{
		return std::basic_string<TCHAR>(static_cast<typename std::basic_string<TCHAR>::const_iterator>(sentry), static_cast<typename std::basic_string<TCHAR>::const_iterator>(m_cur));
	}
	StreamException unexpected(TCHAR ch)
	{
		std::ostringstream stream;

		stream << "Line " << m_line << ", Pos " << m_pos << ": Unexpected character " << std::hex << (int)ch << std::dec;

		return StreamException(stream.str());
	}
	StreamException toomany(int count)
	{
		std::ostringstream stream;

		stream << "Line " << m_line << ", Pos " << m_pos << ": The number of rules exceeded " << std::dec << count;

		return StreamException(stream.str());
	}
};
using Stream = MemoryStream<char16_t>;
using WideStream = GenericStream<wchar_t>;
using WideMemoryStream = MemoryStream<wchar_t>;
}