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

class SlaveASTRingBuffer
{
    HANDLE m_handle;
public:
    SlaveASTRingBuffer(LPCTSTR name)
    {
        m_handle = OpenFileMapping(PAGE_READONLY, FALSE, name);

        Assert::AreNotEqual((void *)NULL, (void *)m_handle);


    }
    virtual ~SlaveASTRingBuffer()
    {
        if (m_handle != NULL)
            CloseHandle(m_handle);
    }
};

class MasterASTRingBuffer
{
    HANDLE m_handle;
    void *m_buffer;
    size_t m_bank_size;
    int m_bank_num, m_current_bank;
public:
    MasterASTRingBuffer(size_t bank_size, int bank_num)
        : m_handle(INVALID_HANDLE_VALUE), m_buffer(NULL), m_bank_size(bank_size), m_bank_num(bank_num)
    {
        size_t buffer_size = bank_size * bank_num;

        m_handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, buffer_size, L"AST Buffer");

        Assert::AreNotEqual((void *)NULL, (void *)m_handle);

        m_buffer = MapViewOfFile(m_handle, FILE_MAP_ALL_ACCESS, 0, 0, buffer_size);

        Assert::AreNotEqual((void *)NULL, m_buffer);
    }
    virtual ~MasterASTRingBuffer()
    {
        if (m_buffer != NULL)
            UnmapViewOfFile(m_buffer);
        if (m_handle != NULL)
            CloseHandle(m_handle);
    }
    void *get_buffer()
    {
        return m_buffer;
    }
};

struct ParserContext
{
    Centaurus::ParserEM64T<char>& parser;
    const void *memory;
    const void *result;
};

DWORD WINAPI ParserRunner(LPVOID param)
{
    ParserContext *context = (ParserContext *)param;

    size_t bank_size = 8 * 1024 * 1024;
    int bank_num = 8;

    MasterASTRingBuffer ring_buffer(bank_size, bank_num);

    context->parser.set_buffer(ring_buffer.get_buffer());

    clock_t start_time = clock();

    context->result = context->parser(context->memory);

    clock_t end_time = clock();

    long flipcount = context->parser.get_flipcount();

    char buf[64];
    snprintf(buf, 64, "Elapsed time = %lf[ms]\r\n", (double)(end_time - start_time) * 1000.0 / CLOCKS_PER_SEC);
    Logger::WriteMessage(buf);

    snprintf(buf, 64, "Flipcount = %ld\r\n", flipcount);
    Logger::WriteMessage(buf);

    ExitThread(0);
}

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

        char *json = LoadTextAligned("C:\\Users\\ihara\\Downloads\\sf-city-lots-json-master\\sf-city-lots-json-master\\citylots.json");

        //char *json = LoadTextAligned("C:\\Users\\ihara\\Downloads\\citylots.json");

        clock_t start_time = clock();

        DryParserContext context{parser, json, NULL};

        HANDLE hThread = CreateThread(NULL, 1024*1024*1024, DryParserRunner, (LPVOID)&context, 0, NULL);

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

        MyErrorHandler errhandler;

        ParserEM64T<char> parser(grammar, &logger, &errhandler);

        //Logger::WriteMessage(logger.getString());

        char *json = LoadTextAligned("C:\\Users\\ihara\\Downloads\\sf-city-lots-json-master\\sf-city-lots-json-master\\citylots.json");

        //char *json = LoadTextAligned("C:\\Users\\ihara\\Downloads\\citylots.json");

        ParserContext context{ parser, json, NULL };

        HANDLE hThread = CreateThread(NULL, 1024 * 1024 * 1024, ParserRunner, (LPVOID)&context, 0, NULL);

        WaitForSingleObject(hThread, INFINITE);

        Assert::AreEqual((const void *)(json + strlen(json)), context.result);

        _aligned_free(json);
    }
};
}
}
}