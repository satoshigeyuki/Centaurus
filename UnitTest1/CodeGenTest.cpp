#include "stdafx.h"
#include "nfa.hpp"
#include "CodeGenEM64T.hpp"

#include <CppUnitTest.h>

namespace Microsoft
{
namespace VisualStudio
{
namespace CppUnitTestFramework
{
TEST_CLASS(CodeGenTest)
{
public:
    TEST_METHOD_INITIALIZE(CodeGenInit)
    {

    }
    TEST_METHOD(DFACodeGenTest1)
    {
        using namespace Centaurus;

        Stream stream(u"(abc)+def");
        NFA<char> nfa(stream);
        DFA<char> dfa(nfa);

        asmjit::JitRuntime rt;

        DFARoutineEM64T<char> dfa_routine(rt, dfa);

        const char buf[] = "abcabcabcdef";
        const char buf2[] = "abcabcde";

        Assert::AreEqual((const void *)(buf + sizeof(buf) - 1), dfa_routine.run(buf));
        Assert::AreEqual((const void *)NULL, dfa_routine.run(buf2));
    }
};
}
}
}