#include <iostream>
#include <sstream>
#include <fstream>

#include "reparser.hpp"

using namespace Centaur;

int main(int argc, const char *argv[])
{
    std::wstring pattern;

    std::wcin >> pattern;

    REPattern<wchar_t> re(pattern);

    std::ofstream of("nfa.dot");

    re.print_nfa(of);

    return 0;
}
