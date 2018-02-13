#include "charclass.hpp"

#include <cctype>
#include <iomanip>

namespace Centaurus
{
template class CharClass<char>;
template class CharClass<char16_t>;
template class CharClass<wchar_t>;

template<typename TCHAR>
void CharClass<TCHAR>::parse(Stream& stream)
{
    char16_t ch;
    bool invert_flag = false;

    ch = stream.get();
    if (ch == u'^')
    {
        invert_flag = true;
        ch = stream.get();
    }

    char16_t start = 0;
    enum
    {
        CC_STATE_START = 0,
        CC_STATE_RANGE,
        CC_STATE_END
    } state = CC_STATE_START;
    for (; ch != u']'; ch = stream.get())
    {
        bool escaped = false;

        if (ch == u'\0' || ch == 0xFFFF)
            throw stream.unexpected(ch);

        if (ch == u'\\')
        {
            ch = stream.get();
            switch (ch)
            {
            case u'\\':
                ch = u'\\';
                break;
            case u'-':
                ch = u'-';
                break;
            case u'[':
                ch = u'[';
                break;
            case u']':
                ch = u']';
                break;
            case u't':
            case u'T':
                ch = u'\t';
                break;
            case u'r':
            case u'R':
                ch = u'\r';
                break;
            case u'n':
            case u'N':
                ch = u'\n';
                break;
            default:
                throw stream.unexpected(ch);
            }
            escaped = true;
        }
        switch (state)
        {
        case CC_STATE_START:
            if (!escaped && ch == u'-')
                throw stream.unexpected(ch);
            start = ch;
            state = CC_STATE_RANGE;
            break;
        case CC_STATE_RANGE:
            if (!escaped && ch == u'-')
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
            if (!escaped && ch == u'-')
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
        throw stream.unexpected(u']');
    }
    if (invert_flag) invert();
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

template<typename TCHAR>
std::ostream& operator<<(std::ostream& os, const CharClass<TCHAR>& cc)
{
    auto i = cc.m_ranges.cbegin();

    for (; i != cc.m_ranges.cend();)
    {
        if (i->end() == i->start() + 1)
        {
            if (i->start() == wide_to_target<TCHAR>(u'"'))
                os << "\\\"";
            else if (i->start() == wide_to_target<TCHAR>(u'\\'))
                os << "\\\\";
            else
            {
                char ch = os.narrow(i->start(), '@');
                os << (std::isprint(ch) ? ch : '@');
            }
        }
        else
        {
            if (i->start() == wide_to_target<TCHAR>(u'"'))
                os << "\\\"";
            else if (i->start() == wide_to_target<TCHAR>(u'\\'))
                os << "\\\\";
            else
            {
                char ch = os.narrow(i->start(), '@');
                os << (std::isprint(ch) ? ch : '@');
            }
            os << '-';
            if (i->end() - 1 == wide_to_target<TCHAR>(u'"'))
                os << "\\\"";
            else if (i->end() - 1 == wide_to_target<TCHAR>(u'\\'))
                os << "\\\\";
            else
            {
                char ch = os.narrow(i->end(), '@');
                os << (std::isprint(ch) ? ch : '@');
            }
        }
        i++;
    }
    return os;
}
template<typename TCHAR>
std::wostream& operator<<(std::wostream& os, const CharClass<TCHAR>& cc)
{
	auto i = cc.m_ranges.cbegin();

	for (; i != cc.m_ranges.cend();)
	{
		if (i->end() == i->start() + 1)
		{
			if (i->start() == wide_to_target<TCHAR>(u'"'))
				os << L"\\\"";
			else if (i->start() == wide_to_target<TCHAR>(u'\\'))
				os << L"\\\\";
			else
			{
				char ch = os.narrow(i->start(), '@');
				os << (std::isprint(ch) ? ch : '@');
			}
		}
		else
		{
			if (i->start() == wide_to_target<TCHAR>(u'"'))
				os << L"\\\"";
			else if (i->start() == wide_to_target<TCHAR>(u'\\'))
				os << L"\\\\";
			else
			{
				char ch = os.narrow(i->start(), '@');
				os << (std::isprint(ch) ? ch : '@');
			}
			os << L'-';
			if (i->end() - 1 == wide_to_target<TCHAR>(u'"'))
				os << L"\\\"";
			else if (i->end() - 1 == wide_to_target<TCHAR>(u'\\'))
				os << L"\\\\";
			else
			{
				char ch = os.narrow(i->end(), '@');
				os << (std::isprint(ch) ? ch : '@');
			}
		}
		i++;
	}
	return os;
}
template std::ostream& operator<<(std::ostream& os, const CharClass<char>& cc);
template std::ostream& operator<<(std::ostream& os, const CharClass<char16_t>& cc);
template std::ostream& operator<<(std::ostream& os, const CharClass<wchar_t>& cc);
template std::wostream& operator<<(std::wostream& os, const CharClass<char>& cc);
template std::wostream& operator<<(std::wostream& os, const CharClass<char16_t>& cc);
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
			template std::wstring ToString(const Centaurus::CharClass<wchar_t>& cc);
		}
	}
}