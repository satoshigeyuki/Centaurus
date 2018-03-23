#include "stdafx.h"
#include <CppUnitTest.h>

#include "NFA.hpp"
#include "CodeGenEM64T.hpp"
#include "CATNLoader.hpp"
#include "MasterParser.hpp"
#include "SlaveParser.hpp"
#include "Input.hpp"

#include <time.h>

namespace Microsoft
{
namespace VisualStudio
{
namespace CppUnitTestFramework
{
class MyErrorHandler : public asmjit::ErrorHandler
{
public:
    virtual bool handleError(asmjit::Error err, const char *msg, asmjit::CodeEmitter *src) override
    {
        Logger::WriteMessage(msg);
        Assert::Fail();
    }
};

TEST_CLASS(CodeGenTest)
{
public:
    TEST_METHOD_INITIALIZE(InitCodeGenTest)
    {
#if defined(CENTAURUS_BUILD_WINDOWS)
        SetCurrentDirectoryA(CENTAURUS_PROJECT_DIR);
#endif
    }
    TEST_METHOD(DFACodeGenTest1)
    {
        using namespace Centaurus;

        Stream stream(u"(abc)+def");
        NFA<char> nfa(stream);
        DFA<char> dfa(nfa);

        FILE *logfile;
#if defined(CENTAURUS_BUILD_WINDOWS)
        fopen_s(&logfile, "dfaroutine.asm", "w");
#else
        logfile = fopen("dfaroutine.asm", "w");
#endif
        asmjit::FileLogger logger(logfile);

        DFARoutineEM64T<char> dfa_routine(dfa);

        fflush(logfile);

        const char buf[] = "abcabcabcdef";
        const char buf2[] = "abcabcde";

        Assert::AreEqual((const void *)(buf + sizeof(buf) - 1), dfa_routine(buf));
        Assert::AreEqual((const void *)NULL, dfa_routine(buf2));
    }
    TEST_METHOD(LDFACodeGenTest1)
    {
        using namespace Centaurus;

        CompositeATN<char> catn = LoadCATN("grammar\\json.cgr");

        LookaheadDFA<char> ldfa(catn, catn.convert_atn_path(ATNPath(u"Object", 0)));

        LDFARoutineEM64T<char> ldfa_routine(ldfa);

        Assert::AreEqual(ldfa.run("{"), ldfa_routine("{"));
    }
    TEST_METHOD(MatchCodeGenTest1)
    {
        using namespace Centaurus;

        const char str1[] = "A quick brown fox jumps over the lazy dog.";

        MatchRoutineEM64T<char> match_routine(str1);

        char *buf = (char *)malloc((sizeof(str1) + 15) / 16 * 16);
        memcpy(buf, str1, sizeof(str1));

        Assert::AreEqual((const void *)(buf + sizeof(str1) - 1), match_routine(buf));
    }
    TEST_METHOD(SkipCodeGenTest1)
    {
        using namespace Centaurus;

        const char str1[] = " \t\n\rabcdefg";

        CharClass<char> filter({' ', '\t', '\r', '\n'});

        asmjit::StringLogger logger;

        SkipRoutineEM64T<char> skip_routine(filter, &logger);

        Logger::WriteMessage(logger.getString());

        Assert::AreEqual((const void *)(str1 + 4), skip_routine(str1));
    }
    TEST_METHOD(DryParserGenTest1)
    {
        using namespace Centaurus;

        Grammar<char> grammar = LoadGrammar("grammar\\json.cgr");

        asmjit::StringLogger logger;

        DryParserEM64T<char> parser(grammar, &logger);

        //Logger::WriteMessage(logger.getString());

        //MappedFileInput json("/home/ihara/Downloads/sf-city-lots-json-master/citylots.json");
        MappedFileInput json("C:\\Users\\ihara\\Downloads\\sf-city-lots-json-master\\sf-city-lots-json-master\\citylots.json");

        MasterParser<DryParserEM64T<char> > runner{parser, json.get_buffer(), NULL};

        runner.run();

        //Assert::AreEqual((const void *)(json + strlen(json)), context.result);
        Assert::IsTrue(runner.get_result() != NULL);

        //_aligned_free(json);
    }
    TEST_METHOD(ParserGenTest1)
    {
        using namespace Centaurus;

        Grammar<char> grammar = LoadGrammar("grammar\\json.cgr");

        asmjit::StringLogger logger;

        MyErrorHandler errhandler;

        ParserEM64T<char> parser(grammar, &logger, &errhandler);
		ChaserEM64T<char> chaser(grammar, &logger, &errhandler);

        size_t bank_size = 8 * 1024 * 1024;
        int bank_num = 8;

        IPCMaster ring_buffer(bank_size, bank_num);

        parser.set_buffer(ring_buffer.request_bank());

        //MappedFileInput json("/home/ihara/Downloads/sf-city-lots-json-master/citylots.json");

        //Logger::WriteMessage(logger.getString());

        MappedFileInput json("C:\\Users\\ihara\\Downloads\\sf-city-lots-json-master\\sf-city-lots-json-master\\citylots.json");

        MasterParser<ParserEM64T<char> > master{ parser, json.get_buffer(), NULL };
		SlaveParser<ChaserEM64T<char>, int> slave{ chaser };

        master.start();
		slave.start();

        long flipcount = parser.get_flipcount();

        char buf[64];
        snprintf(buf, 64, "Flipcount = %ld\r\n", flipcount);
        Logger::WriteMessage(buf);

        //Assert::AreEqual((const void *)(json + strlen(json)), context.result);
        Assert::IsTrue(runner.get_result() != NULL);

        //_aligned_free(json);
    }
};
}
}
}
