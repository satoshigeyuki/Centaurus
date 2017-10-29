#include <iostream>
#include <sstream>
#include <fstream>
#include <locale>
#include <codecvt>

#include "stream.hpp"
#include "grammar.hpp"

int main(int argc, const char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Please specify a grammar file." << std::endl;
        return -1;
    }

    std::cout << "Centaurus parser generator v0.1" << std::endl;

    std::ifstream grammar_file(argv[1], std::ios::in);

    std::string raw_grammar(std::istreambuf_iterator<char>(grammar_file), {});

    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> wide_converter;

    std::wstring wide_grammar = wide_converter.from_bytes(raw_grammar);

    Centaurus::Stream stream(std::move(wide_grammar));

    Centaurus::Grammar<char> grammar;

    try
    {
        grammar.parse(stream);
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        std::cerr << "ATN construction failed." << std::endl;
        return -1;
    }

    std::ofstream graph("atn.dot");

    grammar.print(graph, L"Object");

    return 0;
}
