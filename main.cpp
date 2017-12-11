#include <iostream>
#include <sstream>
#include <fstream>
#include <locale>
#include <codecvt>

#include "stream.hpp"
#include "grammar.hpp"

#include "catn2.hpp"
#include "ldfa.hpp"

#include "codegen.hpp"

int main(int argc, const char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Please specify a grammar file." << std::endl;
        return -1;
    }

    //std::cout << "Centaurus parser generator v0.1" << std::endl;

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
 
    //Centaurus::CompositeATN<char> catn = grammar.build_catn();

    Centaurus::CompositeATN<char> catn(grammar);

    std::ofstream catn_graph("catn.dot");

    catn[u"ListContent"].print(catn_graph, "CATN");

    Centaurus::LookaheadDFA<char> ldfa(catn, Centaurus::ATNPath(u"ListItems", 0));

    return 0;
}
