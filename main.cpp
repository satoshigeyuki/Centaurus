#include <iostream>
#include <sstream>

#include "reparser.hpp"

using namespace Centaur;

int main(int argc, const char *argv[])
{
    std::wstring pattern = L"ABCDEFG";

    REPattern<char16_t> re(pattern);

    return 0;
}
