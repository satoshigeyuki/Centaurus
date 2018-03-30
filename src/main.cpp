#include <iostream>
#include <sstream>
#include <fstream>
#include <locale>
#include <codecvt>

#include "Stream.hpp"
#include "Grammar.hpp"

#include "CompositeATN.hpp"
#include "LookaheadDFA.hpp"

std::locale::id std::codecvt<char, char, std::mbstate_t>::id;
std::locale::id std::codecvt<char16_t, char, std::mbstate_t>::id;
std::locale::id std::codecvt<char32_t, char, std::mbstate_t>::id;
//std::locale::id std::codecvt<wchar_t, char, std::mbstate_t>::id;

int main(int argc, const char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Please specify a grammar file." << std::endl;
        return -1;
    }

    std::ifstream grammar_file(argv[1], std::ios::in);

    std::string raw_grammar(std::istreambuf_iterator<char>(grammar_file), {});

    std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> wide_converter;

    std::u16string wide_grammar = wide_converter.from_bytes(raw_grammar);

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

    grammar.print(graph, u"Object");
 
    Centaurus::CompositeATN<char> catn(grammar);

    std::ofstream catn_graph("catn.dot");

    return 0;
}
