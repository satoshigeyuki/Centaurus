#include "atn.hpp"

namespace Centaurus
{
template class ATN<char>;
template class ATN<wchar_t>;

template<typename TCHAR>
void ATNNode<TCHAR>::parse_literal(Stream& stream)
{
    wchar_t leader = stream.get();

    wchar_t ch = stream.get();

    for (; ch != L'\0' && ch != L'\'' && ch != L'"'; ch = stream.get())
    {
        m_literal.push_back(wide_to_target<TCHAR>(ch));
    }

    if (leader != ch)
        throw stream.unexpected(ch);
}
}
