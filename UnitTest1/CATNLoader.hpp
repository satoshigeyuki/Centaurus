#pragma once

#include "grammar.hpp"
#include "catn2.hpp"

#include "CppUnitTest.h"

static Centaurus::CompositeATN<char> LoadCATN(const char *filename)
{
    using namespace Microsoft::VisualStudio::CppUnitTestFramework;

    std::ifstream grammar_file(filename, std::ios::in);

    Assert::IsTrue(grammar_file.is_open(), L"Failed to open grammar file.");

    std::string raw_grammar(std::istreambuf_iterator<char>(grammar_file), {});

    std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> wide_converter;

    std::u16string wide_grammar = wide_converter.from_bytes(raw_grammar);

    std::string narrow_grammar = wide_converter.to_bytes(wide_grammar);

    Assert::IsTrue(raw_grammar.compare(narrow_grammar) == 0, L"Conversion to UCS-2 failed.");

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