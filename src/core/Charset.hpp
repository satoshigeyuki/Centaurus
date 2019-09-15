#pragma once

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
	case L'\n':
	case L'\u0085':
	case L'\u2028':
	case L'\u2029':
		return '\n';
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
	case u'\n':
	case u'\u0085':
	case u'\u2028':
	case u'\u2029':
		return '\n';
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
	case U'\n':
	case U'\u0085':
	case U'\u2028':
	case U'\u2029':
		return '\n';
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
	case '/': return L'/';
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
	case '/': return u'/';
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
	case '/': return U'/';
	default:
		return static_cast<char32_t>(ch);
	}
}
}