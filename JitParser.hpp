#pragma once

#include "grammar.hpp"

#include "asmjit/asmjit.h"

namespace Centaurus
{
template<typename TCHAR>
class JitParser
{
    asmjit::JitRuntime runtime;
public:
    JitParser(const Grammar<TCHAR>& grammar);
    virtual ~JitParser();
    bool just_parse(const wchar_t *filename);
};
}