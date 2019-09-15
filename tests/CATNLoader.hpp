#pragma once

#include "Grammar.hpp"
#include "CompositeATN.hpp"

#include "CppUnitTest.h"

static Centaurus::CompositeATN<char> LoadCATN(const char *filename)
{
    using namespace Microsoft::VisualStudio::CppUnitTestFramework;

    Centaurus::Grammar<char> grammar;

    try
    {
        grammar.parse(filename);
    }
    catch (const std::exception& ex)
    {
        Assert::Fail(L"ATN construction failed.");
    }

    Centaurus::CompositeATN<char> catn(grammar);

    return catn;
}

template<typename TCHAR>
static Centaurus::Grammar<TCHAR> LoadGrammar(const char *filename)
{
    using namespace Microsoft::VisualStudio::CppUnitTestFramework;

    Centaurus::Grammar<TCHAR> grammar;

    try
    {
        grammar.parse(filename);
    }
    catch (const std::exception& ex)
    {
        Assert::Fail(L"ATN construction failed.");
    }

    return grammar;
}