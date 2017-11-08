#include "charclass.hpp"

#include <cctype>

namespace Centaurus
{
template class CharClass<char>;
template class CharClass<wchar_t>;

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

template std::ostream& operator<<(std::ostream& os, const CharClass<char>& cc);
template std::ostream& operator<<(std::ostream& os, const CharClass<wchar_t>& cc);

template<typename TCHAR>
std::ostream& operator<<(std::ostream& os, const CharClass<TCHAR>& cc)
{
    auto i = cc.m_ranges.cbegin();

    for (; i != cc.m_ranges.cend();)
    {
        if (i->end() == i->start() + 1)
        {
            if (i->start() == wide_to_target<TCHAR>(L'"'))
                os << "\\\"";
            else if (i->start() == wide_to_target<TCHAR>(L'\\'))
                os << "\\\\";
            else
            {
                char ch = os.narrow(i->start(), '@');
                os << (std::isprint(ch) ? ch : '@');
            }
        }
        else
        {
            if (i->start() == wide_to_target<TCHAR>(L'"'))
                os << "\\\"";
            else if (i->start() == wide_to_target<TCHAR>(L'\\'))
                os << "\\\\";
            else
            {
                char ch = os.narrow(i->start(), '@');
                os << (std::isprint(ch) ? ch : '@');
            }
            os << '-';
            if (i->end() - 1 == wide_to_target<TCHAR>(L'"'))
                os << "\\\"";
            else if (i->end() - 1 == wide_to_target<TCHAR>(L'\\'))
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
}
