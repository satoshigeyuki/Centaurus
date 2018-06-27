/*!
 * @file Grammar.cpp
 * @brief Public API for Grammar class
 */
#include "Grammar.hpp"

using namespace Centaurus;

extern "C" void *GrammarCreate()
{
    return new Grammar();
}

extern "C" void GrammarDestroy(Grammar *grammar)
{
    delete grammar;
}

extern "C" void GrammarAddMachine(Grammar *grammar, const wchar_t *name, void *machine)
{
    
}
