#include "stdafx.h"
#include "nfa.hpp"
#include "CodeGenEM64T.hpp"
#include <setjmp.h>
#include "CATNLoader.hpp"
#include "TestDataLoader.hpp"

#include <time.h>
#include <CppUnitTest.h>

namespace Microsoft
{
namespace VisualStudio
{
namespace CppUnitTestFramework
{
struct DryParserContext
{
    Centaurus::DryParserEM64T<char>& parser;
    const void *memory;
    const void *result;
};
DWORD WINAPI DryParserRunner(LPVOID param)
{
    DryParserContext *context = (DryParserContext *)param;

    context->result = context->parser(context->memory);

    ExitThread(0);
}

//Pointer to the end of the AST buffer.
__declspec(thread) void *buffer_eptr;
//Pointer to the pointer to the current AST output position.
__declspec(thread) void **buffer_pptr;

void ParserSignalHandler(unsigned int code, struct _EXCEPTION_POINTERS *pointers)
{
    struct _EXCEPTION_RECORD *record = pointers->ExceptionRecord;

    if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
    {
        ULONG_PTR violation_flags = record->ExceptionInformation[0];
        ULONG_PTR violation_address = record->ExceptionInformation[1];

        if (violation_flags == (ULONG_PTR)1)
        {
            if ((void *)violation_address == buffer_eptr)
            {
                Logger::WriteMessage("Buffer overrun detected.");
            }
        }
    }
}

struct ParserContext
{
    Centaurus::ParserEM64T<char>& parser;
    const void *memory;
    const void *result;
};

DWORD WINAPI ParserRunner(LPVOID param)
{
    ParserContext *context = (ParserContext *)param;

    _set_se_translator(ParserSignalHandler);

    size_t buffer_size = 1048576;
    char *buffer = (char *)malloc(buffer_size);

    //Set values used by the SE translator to TLS
    buffer_eptr = buffer + buffer_size;
    buffer_pptr = context->parser.set_ast_buffer(buffer);

    __try
    {
        context->result = context->parser(context->memory);
    }
    __finally
    {
        Logger::WriteMessage("Parser execution completed");
    }

    free(buffer);

    ExitThread(0);
}

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

        CompositeATN<char> catn = LoadCATN("../../../json.cgr");

        LookaheadDFA<char> ldfa(catn, ATNPath(u"Object", 0));

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

        Grammar<char> grammar = LoadGrammar("../../../json.cgr");

        asmjit::StringLogger logger;

        DryParserEM64T<char> parser(grammar, &logger);

        //Logger::WriteMessage(logger.getString());

        //char *json = LoadTextAligned("C:\\Users\\ihara\\Downloads\\sf-city-lots-json-master\\sf-city-lots-json-master\\citylots.json");

        char *json = LoadTextAligned("C:\\Users\\ihara\\Documents\\test1.json");

        clock_t start_time = clock();

        DryParserContext context{parser, json, NULL};

        HANDLE hThread = CreateThread(NULL, 256*1024*1024, DryParserRunner, (LPVOID)&context, 0, NULL);

        WaitForSingleObject(hThread, INFINITE);

        clock_t end_time = clock();

        char buf[64];
        snprintf(buf, 64, "Elapsed time = %lf[ms]\r\n", (double)(end_time - start_time) * 1000.0 / CLOCKS_PER_SEC);
        Logger::WriteMessage(buf);

        Assert::AreEqual((const void *)(json + strlen(json)), context.result);

        _aligned_free(json);
    }
    TEST_METHOD(ParserGenTest1)
    {
        using namespace Centaurus;

        Grammar<char> grammar = LoadGrammar("../../../json.cgr");

        asmjit::StringLogger logger;

        ParserEM64T<char> parser(grammar, &logger);

        //Logger::WriteMessage(logger.getString());

        //char *json = LoadTextAligned("C:\\Users\\ihara\\Downloads\\sf-city-lots-json-master\\sf-city-lots-json-master\\citylots.json");

        char *json = LoadTextAligned("C:\\Users\\ihara\\Documents\\test1.json");

        clock_t start_time = clock();

        ParserContext context{ parser, json, NULL };

        HANDLE hThread = CreateThread(NULL, 256 * 1024 * 1024, ParserRunner, (LPVOID)&context, 0, NULL);

        WaitForSingleObject(hThread, INFINITE);

        clock_t end_time = clock();

        char buf[64];
        snprintf(buf, 64, "Elapsed time = %lf[ms]\r\n", (double)(end_time - start_time) * 1000.0 / CLOCKS_PER_SEC);
        Logger::WriteMessage(buf);

        Assert::AreEqual((const void *)(json + strlen(json)), context.result);

        _aligned_free(json);
    }
};
}
}
}