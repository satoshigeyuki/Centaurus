#pragma once

namespace Centaur
{
template<typename TCHAR> TCHAR wide_to_target(wchar_t ch)
{
    return static_cast<TCHAR>(ch);
}
}
