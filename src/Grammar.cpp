#include "CompositeATN.hpp"
#include "LookaheadDFA.hpp"

namespace Centaurus
{
template<typename TCHAR>
void Grammar<TCHAR>::print_catn(std::wostream& os, const Identifier& id) const
{
    CompositeATN<TCHAR> catn(*this);

    catn[id].print(os, id.str());
}
template<typename TCHAR>
void Grammar<TCHAR>::print_ldfa(std::wostream& os, const ATNPath& path) const
{
    CompositeATN<TCHAR> catn(*this);

    LookaheadDFA<TCHAR> ldfa(catn, path);

    ldfa.print(os, path.leaf_id().str());
}
template class Grammar<char>;
template class Grammar<unsigned char>;
template class Grammar<wchar_t>;
}
