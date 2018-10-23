#include "stdafx.h"
#include <CppUnitTest.h>

#include "NFA.hpp"
#include "CodeGenEM64T.hpp"
#include "CATNLoader.hpp"
#include "Stage1Runner.hpp"
#include "Stage2Runner.hpp"
#include "Stage3Runner.hpp"

#include <time.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

static uint64_t get_us_clock()
{
#if defined(CENTAURUS_BUILD_LINUX)
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
#elif defined(CENTAURUS_BUILD_WINDOWS)
	LARGE_INTEGER qpc;
	QueryPerformanceCounter(&qpc);
	LARGE_INTEGER qpf;
	QueryPerformanceFrequency(&qpf);
	return (uint64_t)qpc.QuadPart * 1000000 / (uint64_t)qpf.QuadPart;
#endif
}

namespace UnitTest1
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

        Stream stream(L"(abc)+def");
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

        LookaheadDFA<char> ldfa(catn, catn.convert_atn_path(ATNPath(L"Object", 0)));

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

        Grammar<unsigned char> grammar = LoadGrammar<unsigned char>("grammar/json2.cgr");

		grammar.optimize();

		/*FILE *logFile;
		fopen_s(&logFile, "parser.asm", "w");
        asmjit::FileLogger logger(logFile);*/
		
        DryParserEM64T<unsigned char> parser(grammar, NULL);

		//fclose(logFile);

        //Logger::WriteMessage(logger.getString());

        //MappedFileInput json("/home/ihara/Downloads/sf-city-lots-json-master/citylots.json");

#if defined(CENTAURUS_BUILD_WINDOWS)
		const char *input_path = "C:\\Users\\ihara\\Downloads\\sf-city-lots-json-master\\sf-city-lots-json-master\\citylots.json";
		//const char *input_path = "test2.json";
#elif defined(CENTAURUS_BUILD_LINUX)
		//const char *input_path = "/mnt/c/Users/ihara/Downloads/sf-city-lots-json-master/sf-city-lots-json-master/citylots.json";
        const char *input_path = "/home/ihara/Downloads/sf-city-lots-json-master/citylots.json";
#endif

        Stage1Runner runner{input_path, &parser, 8 * 1024 * 1024, 8};

		uint64_t start_time = get_us_clock();

        runner.start();
        runner.wait();

		uint64_t end_time = get_us_clock();

		char msg[256];
		sprintf_s(msg, "Elapsed time: %lu[ms]\r\n", (end_time - start_time) / 1000);
		Logger::WriteMessage(msg);

        //Assert::AreEqual((const void *)(json + strlen(json)), context.result);
        Assert::IsTrue(runner.get_result() != NULL);

        //_aligned_free(json);
    }
    TEST_METHOD(ParserGenTest1)
    {
        using namespace Centaurus;

        Grammar<unsigned char> grammar = LoadGrammar<unsigned char>("grammar/json2.cgr");

        asmjit::StringLogger logger;

        MyErrorHandler errhandler;

        ParserEM64T<unsigned char> parser(grammar, &logger, &errhandler);

#if defined(CENTAURUS_BUILD_WINDOWS)
		const char *input_path = "C:\\Users\\ihara\\Downloads\\sf-city-lots-json-master\\sf-city-lots-json-master\\citylots.json";
#elif defined(CENTAURUS_BUILD_LINUX)
		//const char *input_path = "/mnt/c/Users/ihara/Downloads/sf-city-lots-json-master/sf-city-lots-json-master/citylots.json";
        const char *input_path = "/home/ihara/Downloads/sf-city-lots-json-master/citylots.json";
#endif

        Stage1Runner runner{input_path, &parser, 8 * 1024 * 1024, 8};

		runner.start();
        runner.wait();

        //Assert::AreEqual((const void *)(json + strlen(json)), context.result);
        //Assert::IsTrue(runner.get_result() != NULL);

        //_aligned_free(json);
    }
    TEST_METHOD(ChaserGenTest1)
    {
        using namespace Centaurus;

        Grammar<unsigned char> grammar = LoadGrammar<unsigned char>("grammar/json2.cgr");

        asmjit::StringLogger logger;

        MyErrorHandler errhandler;

        ParserEM64T<unsigned char> parser(grammar, &logger, &errhandler);
        ChaserEM64T<unsigned char> chaser(grammar);

#if defined(CENTAURUS_BUILD_WINDOWS)
		//const char *input_path = "test2.json";
		const char *input_path = "C:\\Users\\ihara\\Downloads\\sf-city-lots-json-master\\sf-city-lots-json-master\\citylots.json";
#elif defined(CENTAURUS_BUILD_LINUX)
        //const char *input_path = "/mnt/c/Users/ihara/Downloads/sf-city-lots-json-master/sf-city-lots-json-master/citylots.json";
        const char *input_path = "/home/ihara/Downloads/sf-city-lots-json-master/citylots.json";
#endif

        Stage1Runner runner1{ input_path, &parser, 8 * 1024 * 1024, 4 };
#if defined(CENTAURUS_BUILD_WINDOWS)
        int pid = GetCurrentProcessId();
#elif defined(CENTAURUS_BUILD_LINUX)
        int pid = getpid();
#endif
        std::vector<Stage2Runner *> st2_runners;
        for (int i = 0; i < 2; i++)
        {
            st2_runners.push_back(new Stage2Runner{input_path, &chaser, 8 * 1024 * 1024, 4, pid});
        }
        Stage3Runner runner3{ input_path, &chaser, 8 * 1024 * 1024, 4, pid };

        runner1.start();
        for (auto p : st2_runners)
        {
            p->start();
        }
        runner3.start();

        runner1.wait();
        for (auto p : st2_runners)
        {
            p->wait();
        }
        runner3.wait();

        //Assert::AreEqual((const void *)(json + strlen(json)), context.result);
        //Assert::IsTrue(runner.get_result() != NULL);

        //_aligned_free(json);
    }
};
}
