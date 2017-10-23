#pragma once

namespace Centaur
{
template<typename TCHAR> TCHAR get_target_char(wchar_t ch)
{
    return static_cast<TCHAR>(ch);
}
}
