#pragma once

#include "Grammar.hpp"
#include "CompositeATN.hpp"

#include "CppUnitTest.h"

static Centaurus::CompositeATN<char> LoadCATN(const char *filename)
{
    using namespace Microsoft::VisualStudio::CppUnitTestFramework;

    std::wifstream grammar_file(filename, std::ios::in);

    Assert::IsTrue(grammar_file.is_open(), L"Failed to open grammar file.");

    std::wstring wide_grammar(std::istreambuf_iterator<wchar_t>(grammar_file), {});

    Centaurus::Stream stream(std::move(wide_grammar));

    Centaurus::Grammar<char> grammar;

    try
    {
        grammar.parse(stream);
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

    std::wifstream grammar_file(filename, std::ios::in);

    Assert::IsTrue(grammar_file.is_open(), L"Failed to open grammar file.");

    std::wstring wide_grammar(std::istreambuf_iterator<wchar_t>(grammar_file), {});

    Centaurus::Stream stream(std::move(wide_grammar));

    Centaurus::Grammar<TCHAR> grammar;

    try
    {
        grammar.parse(stream);
    }
    catch (const std::exception& ex)
    {
        Assert::Fail(L"ATN construction failed.");
    }

    return grammar;
}