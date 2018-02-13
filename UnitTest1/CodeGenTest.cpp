#include "stdafx.h"
#include "nfa.hpp"
#include "CodeGenEM64T.hpp"
#include <setjmp.h>
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

        FILE *logfile;
        fopen_s(&logfile, "dfaroutine.asm", "w");
        asmjit::FileLogger logger(logfile);

        DFARoutineEM64T<char> dfa_routine(dfa, &logger);

        fflush(logfile);

        const char buf[] = "abcabcabcdef";
        const char buf2[] = "abcabcde";

        int result;
        jmp_buf jb;
        if ((result = setjmp(jb)) == 0)
        {
            Assert::AreEqual((const void *)(buf + sizeof(buf) - 1), dfa_routine(buf, jb));
            dfa_routine(buf2, jb);
        }
        else
        {
            Assert::AreEqual(1, result);
        }
    }
    TEST_METHOD(LDFACodeGenTest1)
    {
        using namespace Centaurus;

        CompositeATN<char> catn = LoadCATN("../../../json.cgr");

        LookaheadDFA<char> ldfa(catn, ATNPath(u"Object", 0));

        LDFARoutineEM64T<char> ldfa_routine(ldfa);

        int result;
        jmp_buf jb;
        if ((result = setjmp(jb)) == 0)
        {
            Assert::IsTrue(ldfa_routine("{", jb) > 0);
        }
        else
        {
            Assert::Fail(L"long jump from the parsing routine.");
        }
    }
    TEST_METHOD(MatchCodeGenTest1)
    {
        using namespace Centaurus;

        const char str1[] = "A quick brown fox jumps over the lazy dog.";

        MatchRoutineEM64T<char> match_routine(str1);

        char *buf = (char *)malloc((sizeof(str1) + 15) / 16 * 16);
        memcpy(buf, str1, sizeof(str1));

        int result;
        jmp_buf jb;
        if ((result = setjmp(jb)) == 0)
        {
            Assert::AreEqual((const void *)(buf + sizeof(str1) - 1), match_routine(buf, jb));
        }
        else
        {
            Assert::Fail(L"long jump from the parsing routine.");
        }
    }
    TEST_METHOD(SkipCodeGenTest1)
    {
        using namespace Centaurus;

        const char str1[] = " \t\n\rabcdefg";

        Stream filter_source(u" \\t\\r\\n]");
        CharClass<char> filter(filter_source);

        Logger::WriteMessage(ToString(filter).c_str());

        SkipRoutineEM64T<char> skip_routine(filter);

        /*int result;
        jmp_buf buf;
        if ((result = setjmp(buf)) == 0)
        {
            Assert::AreEqual((const void *)(str1 + sizeof(str1) - 1), skip_routine(str1));
        }
        else
        {
            Assert::Fail(L"long jump from the parsing routine.");
        }*/
    }
};
}
}
}