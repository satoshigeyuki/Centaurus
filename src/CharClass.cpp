#include "CharClass.hpp"

#include <cctype>
#include <iomanip>

namespace Centaurus
{
template class CharClass<char>;
template class CharClass<unsigned char>;
template class CharClass<wchar_t>;

template<typename TCHAR>
void CharClass<TCHAR>::parse(Stream& stream)
{
    wchar_t ch;
    bool invert_flag = false;

    ch = stream.get();
    if (ch == L'^')
    {
        invert_flag = true;
        ch = stream.get();
    }

    wchar_t start = 0;
    enum
    {
        CC_STATE_START = 0,
        CC_STATE_RANGE,
        CC_STATE_END
    } state = CC_STATE_START;
    for (; ch != L']'; ch = stream.get())
    {
        bool escaped = false;

        if (ch == L'\0' || ch == 0xFFFF)
            throw stream.unexpected(ch);

        if (ch == L'\\')
        {
            ch = stream.get();
            switch (ch)
            {
            case L'\\':
                ch = L'\\';
                break;
            case L'-':
                ch = L'-';
                break;
            case L'[':
                ch = L'[';
                break;
            case L']':
                ch = L']';
                break;
            case L't':
            case L'T':
                ch = L'\t';
                break;
            case L'r':
            case L'R':
                ch = L'\r';
                break;
            case L'n':
            case L'N':
                ch = L'\n';
                break;
            default:
                throw stream.unexpected(ch);
            }
            escaped = true;
        }
        switch (state)
        {
        case CC_STATE_START:
            if (!escaped && ch == L'-')
                throw stream.unexpected(ch);
            start = ch;
            state = CC_STATE_RANGE;
            break;
        case CC_STATE_RANGE:
            if (!escaped && ch == L'-')
            {
                state = CC_STATE_END;
            }
            else
            {
                *this |= Range<TCHAR>(start, start + 1);
                start = ch;
                state = CC_STATE_RANGE;
            }
            break;
        case CC_STATE_END:
            if (!escaped && ch == L'-')
            {
                throw stream.unexpected(ch);
            }
            else
            {
                *this |= Range<TCHAR>(start, ch + 1);
                state = CC_STATE_START;
            }
            break;
        }
    }
    if (state == CC_STATE_RANGE)
    {
        *this |= Range<TCHAR>(start, start + 1);
    }
    else if (state == CC_STATE_END)
    {
        throw stream.unexpected(L']');
    }

	std::sort(m_ranges.begin(), m_ranges.end());

	if (invert_flag)
	{
		*this = this->operator~();
	}
}

template<typename TCHAR>
CharClass<TCHAR> CharClass<TCHAR>::operator|(const CharClass<TCHAR>& cc) const
{
    CharClass<TCHAR> new_class;

    auto i = m_ranges.cbegin();
    auto j = cc.m_ranges.cbegin();

    Range<TCHAR> ri, rj;

    if (i != m_ranges.cend()) ri = *i;
    if (j != cc.m_ranges.cend()) rj = *j;

    while (i != m_ranges.cend() && j != cc.m_ranges.cend())
    {
        if (ri < rj)
        {
            new_class.m_ranges.push_back(ri);
            if (++i != m_ranges.cend())
                ri = *i;
        }
        else if (ri > rj)
        {
            new_class.m_ranges.push_back(rj);
            if (++j != cc.m_ranges.cend())
                rj = *j;
        }
        else
        {
            if (ri.end() < rj.end())
            {
                rj = rj.merge(ri);
                if (++i != m_ranges.cend())
                    ri = *i;
            }
            else
            {
                ri = ri.merge(rj);
                if (++j != cc.m_ranges.cend())
                    rj = *j;
            }
        }
    }

    new_class.m_ranges.insert(new_class.m_ranges.end(), i, m_ranges.cend());
    new_class.m_ranges.insert(new_class.m_ranges.end(), j, cc.m_ranges.cend());

    return new_class;
}

template<typename TCHAR>
CharClass<TCHAR> CharClass<TCHAR>::operator&(const CharClass<TCHAR>& cc) const
{
    CharClass<TCHAR> new_class;

    auto i = m_ranges.cbegin();
    auto j = cc.m_ranges.cbegin();

    Range<TCHAR> ri, rj;

    if (i != m_ranges.cend()) ri = *i;
    if (j != cc.m_ranges.cend()) rj = *j;

    while (i != m_ranges.cend() && j != cc.m_ranges.cend())
    {
        if (ri < rj)
        {
            if (++i != m_ranges.cend())
                ri = *i;
        }
        else if (ri > rj)
        {
            if (++j != cc.m_ranges.cend())
                rj = *j;
        }
        else
        {
            new_class.m_ranges.push_back(ri.intersect(rj));
            if (ri.end() < rj.end())
            {
                if (++i != m_ranges.cend())
                    ri = *i;
            }
            else
            {
                if (++j != cc.m_ranges.cend())
                    rj = *j;
            }
        }
    }

    return new_class;
}

static std::wostream& printc(std::wostream& os, char ch)
{
	if (!isgraph(ch))
	{
		wchar_t buf[8];
		swprintf(buf, 8, L"\\x%0X", (int)ch);
		return os << buf;
	}
	else
	{
		switch (ch)
		{
		case '"':
			return os << L"\\\"";
		case '\\':
			return os << L"\\\\";
		default:
			return os << os.widen(ch);
		}
	}
}
static std::wostream& printc(std::wostream& os, unsigned char ch)
{
	return printc(os, (char)ch);
}
static std::wostream& printc(std::wostream& os, wchar_t ch)
{
	if (!iswgraph(ch))
	{
		wchar_t buf[8];
		swprintf(buf, 8, L"\\x%0X", (wint_t)ch);
		return os << buf;
	}
	else
	{
		switch (ch)
		{
		case L'"':
			return os << L"\\\"";
		case L'\\':
			return os << L"\\\\";
		default:
			return os << ch;
		}
	}
}
static std::wostream& printc(std::wostream& os, char16_t ch)
{
	wchar_t buf[8];
	swprintf(buf, 8, L"\\x%0X", ch);
	return os << buf;
}
template<typename TCHAR>
std::wostream& operator<<(std::wostream& os, const CharClass<TCHAR>& cc)
{
    auto i = cc.m_ranges.cbegin();

    for (; i != cc.m_ranges.cend();)
    {
        if (i->end() == i->start() + 1)
        {
            /*if (i->start() == wide_to_target<TCHAR>(L'"'))
                os << L"\\\"";
            else if (i->start() == wide_to_target<TCHAR>(L'\\'))
                os << L"\\\\";
            else
            {
                wchar_t ch = os.widen(i->start(), '@');
                os << (std::isprint(ch) ? ch : '@');
            }*/
			printc(os, i->start());
        }
        else
        {
			printc(os, i->start());
            os << L'-';
			printc(os, (TCHAR)(i->end() - 1));
        }
        i++;
    }
    return os;
}
template std::wostream& operator<<(std::wostream& os, const CharClass<char>& cc);
template std::wostream& operator<<(std::wostream& os, const CharClass<unsigned char>& cc);
template std::wostream& operator<<(std::wostream& os, const CharClass<wchar_t>& cc);
}

namespace Microsoft
{
    namespace VisualStudio
    {
        namespace CppUnitTestFramework
        {
            template<typename TCHAR>
            std::wstring ToString(const Centaurus::CharClass<TCHAR>& cc)
            {
                std::wostringstream os;
                
                os << std::hex << std::setfill(L'0') << std::setw(sizeof(TCHAR) * 2) << std::showbase;
                os.put(L'[');
                for (const auto& r : cc)
                {
                    if (r.start() + 1 == r.end())
                        os << (unsigned int)r.start();
                    else
                        os << (unsigned int)r.start() << L'-' << (unsigned int)r.end() - 1;
                    os << L' ';
                }
                os.put(L']');

                return os.str();
            }

            template std::wstring ToString(const Centaurus::CharClass<char>& cc);
			template std::wstring ToString(const Centaurus::CharClass<unsigned char>& cc);
            template std::wstring ToString(const Centaurus::CharClass<wchar_t>& cc);
        }
    }
}
