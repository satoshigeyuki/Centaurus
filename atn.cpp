#include "atn.hpp"

namespace Centaurus
{
template class ATN<char>;
template class ATN<char16_t>;

template<typename TCHAR>
void ATNNode<TCHAR>::parse_literal(Stream& stream)
{
    char16_t leader = stream.get();

    char16_t ch = stream.get();

    for (; ch != u'\0' && ch != u'\'' && ch != u'"'; ch = stream.get())
    {
        m_literal.push_back(wide_to_target<TCHAR>(ch));
    }

    if (leader != ch)
        throw stream.unexpected(ch);
}
}
