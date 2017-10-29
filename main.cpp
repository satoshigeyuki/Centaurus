#include <iostream>
#include <sstream>
#include <fstream>

#include "atn.hpp"

using namespace Centaurus;

int main(int argc, const char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Please specify a grammar file." << std::endl;
        return -1;
    }

    std::ifstream grammar_file(argv[1], std::ios::in);

    std::string raw_grammar(std::istreambuf_iterator<char>(grammar_file), {});

    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> wide_converter;

    std::wstring grammar = wide_converter.from_bytes(raw_grammar);

    ATN atn(grammar);

    atn.print(std::cout);

    return 0;
}
