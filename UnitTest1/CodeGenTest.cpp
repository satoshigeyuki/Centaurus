#include "stdafx.h"
#include "nfa.hpp"
#include "CodeGenEM64T.hpp"

#include "CATNLoader.hpp"

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

        DFARoutineEM64T<char> dfa_routine(rt.getCodeInfo(), dfa);

        const char buf[] = "abcabcabcdef";
        const char buf2[] = "abcabcde";

        DFARoutine routine = dfa_routine.getRoutine(rt);

        Assert::AreEqual((const void *)(buf + sizeof(buf) - 1), routine(buf));
        Assert::AreEqual((const void *)NULL, routine(buf2));
    }
    TEST_METHOD(LDFACodeGenTest1)
    {
        using namespace Centaurus;

        CompositeATN<char> catn = LoadCATN("../../../json.cgr");

        LookaheadDFA<char> ldfa(catn, ATNPath(u"Object", 0));

        asmjit::JitRuntime rt;

        LDFARoutineEM64T<char> ldfa_routine(rt.getCodeInfo(), ldfa);

        LDFARoutine routine = ldfa_routine.getRoutine(rt);

        Assert::AreEqual(1, routine("8"));
    }
    TEST_METHOD(MatchCodeGenTest1)
    {
        using namespace Centaurus;

        asmjit::JitRuntime rt;

        const char str1[] = "A quick brown fox jumps over the lazy dog.";

        MatchRoutineEM64T<char> match_routine(rt.getCodeInfo(), str1);

        MatchRoutine routine = match_routine.getRoutine(rt);

        char *buf = (char *)malloc((sizeof(str1) + 15) / 16 * 16);
        memcpy(buf, str1, sizeof(str1));

        Assert::AreEqual((const void *)(str1 + sizeof(str1) - 1), routine(str1));
    }
    TEST_METHOD(SkipCodeGenTest1)
    {
        using namespace Centaurus;

        asmjit::JitRuntime rt;

        const char str1[] = " \t\n\rabcdefg";

        Stream filter_source(u" \\t\\r\\n]");
        CharClass<char> filter(filter_source);

        Logger::WriteMessage(ToString(filter).c_str());

        SkipRoutineEM64T<char> skip_routine(rt.getCodeInfo(), filter);

        SkipRoutine routine = skip_routine.getRoutine(rt);

        Assert::AreEqual((const void *)(str1 + 4), routine(str1));
    }
};
}
}
}