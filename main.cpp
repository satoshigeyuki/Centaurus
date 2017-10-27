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

    std::ofstream nfa_dump("nfa.dot");
    std::ofstream dfa_dump("dfa.dot");

    re.print_nfa(nfa_dump);
    re.print_dfa(dfa_dump);

    return 0;
}
