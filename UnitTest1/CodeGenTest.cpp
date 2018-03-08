#include "stdafx.h"
#include <CppUnitTest.h>
#include "NFA.hpp"
#include "CodeGenEM64T.hpp"
#include <setjmp.h>
#include "CATNLoader.hpp"
#include "IPCMaster.hpp"

#include <time.h>

#if defined(CENTAURUS_BUILD_WINDOWS)
#elif defined(CENTAURUS_BUILD_LINUX)
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace Microsoft
{
namespace VisualStudio
{
namespace CppUnitTestFramework
{
template<class T>
class ParserRunner
{
    T& m_parser;
    const void *m_memory, *m_result;
private:
#if defined(CENTAURUS_BUILD_WINDOWS)
    static DWORD WINAPI thread_runner(LPVOID param)
    {
        ParserRunner<T> *instance = reinterpret_cast<ParserRunner<T> *>(param);

        instance->m_result = instance->m_parser(instance->m_memory);

        ExitThread(0);
    }
#elif defined(CENTAURUS_BUILD_LINUX)
    static void *thread_runner(void *param)
    {
        ParserRunner<T> *instance = reinterpret_cast<ParserRunner<T> *>(param);

        instance->m_result = instance->m_parser(instance->m_memory);
    }
#endif
public:
    ParserRunner(T& parser, const void *memory, const void *result)
        : m_parser(parser), m_memory(memory), m_result(result)
    {
    }
    void run()
    {
        clock_t start_time = clock();

#if defined(CENTAURUS_BUILD_WINDOWS)
        HANDLE hThread = CreateThread(NULL, 256*1024*1024, ParserRunner<T>::thread_runner, (LPVOID)this, 0, NULL);

        WaitForSingleObject(hThread, INFINITE);
#elif defined(CENTAURUS_BUILD_LINUX)
        pthread_t thread;
        pthread_attr_t attr;

        pthread_attr_init(&attr);

        pthread_attr_setstacksize(&attr, 256 * 1024 * 1024);

        pthread_create(&thread, &attr, ParserRunner<T>::thread_runner, this);

        pthread_join(thread, NULL);
#endif

        clock_t end_time = clock();

        char buf[64];
        snprintf(buf, 64, "Elapsed time = %lf[ms]\r\n", (double)(end_time - start_time) * 1000.0 / CLOCKS_PER_SEC);
        Logger::WriteMessage(buf);
    }
    const void *get_result()
    {
        return m_result;
    }
};

class MyErrorHandler : public asmjit::ErrorHandler
{
public:
    virtual bool handleError(asmjit::Error err, const char *msg, asmjit::CodeEmitter *src) override
    {
        Logger::WriteMessage(msg);
        Assert::Fail();
    }
};

class MemoryInput
{
#if defined(CENTAURUS_BUILD_WINDOWS)
    HANDLE hFile, hMapping;
#elif defined(CENTAURUS_BUILD_LINUX)
    int fd;
#endif
    void *m_buffer;
    size_t m_length;
public:
    MemoryInput(const char *filename)
    {
#if defined(CENTAURUS_BUILD_WINDOWS)
        hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

        Assert::AreNotEqual(hFile, INVALID_HANDLE_VALUE, L"Failed to open text file.");

        DWORD dwFileSizeHigh;
        DWORD dwFileSizeLow = GetFileSize(hFile, &dwFileSizeHigh);

        hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, dwFileSizeHigh, dwFileSizeLow, NULL);

        Assert::IsTrue(hMapping != NULL, L"Failed to open a new file mapping.");

        m_length = ((size_t)dwFileSizeHigh << 32) | dwFileSizeLow;

        m_buffer = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
#elif defined(CENTAURUS_BUILD_LINUX)
        fd = open(filename, O_RDONLY);

        struct stat sb;

        fstat(fd, &sb);

        m_length = sb.st_size;

        m_buffer = mmap(NULL, m_length, PROT_READ, MAP_PRIVATE, fd, 0);
#endif
        Assert::IsTrue(m_buffer != NULL, L"Failed to obtain a mapped view of text file.");
    }
    virtual ~MemoryInput()
    {
#if defined(CENTAURUS_BUILD_WINDOWS)
        if (m_buffer != NULL)
            UnmapViewOfFile(m_buffer);
        if (hMapping != NULL)
            CloseHandle(hMapping);
        if (hFile != NULL)
            CloseHandle(hFile);
#elif defined(CENTAURUS_BUILD_LINUX)
        if (m_buffer != NULL)
            munmap(m_buffer, m_length);
        if (fd != -1)
            close(fd);
#endif
    }
    const void *get_buffer()
    {
        return m_buffer;
    }
};
TEST_CLASS(CodeGenTest)
{
public:
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

        CompositeATN<char> catn = LoadCATN("json.cgr");

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

        Grammar<char> grammar = LoadGrammar("json.cgr");

        asmjit::StringLogger logger;

        DryParserEM64T<char> parser(grammar, &logger);

        //Logger::WriteMessage(logger.getString());

        //char *json = LoadTextAligned("C:\\Users\\ihara\\Downloads\\sf-city-lots-json-master\\sf-city-lots-json-master\\citylots.json");

        //char *json = LoadTextAligned("C:\\Users\\ihara\\Downloads\\citylots.json");

        //char *json = LoadTextAligned("..\\..\\..\\test2.json");

        MemoryInput json("/home/ihara/Downloads/sf-city-lots-json-master/citylots.json");

        ParserRunner<DryParserEM64T<char> > runner{parser, json.get_buffer(), NULL};

        runner.run();

        //Assert::AreEqual((const void *)(json + strlen(json)), context.result);
        Assert::IsTrue(runner.get_result() != NULL);

        //_aligned_free(json);
    }
    TEST_METHOD(ParserGenTest1)
    {
        using namespace Centaurus;

        Grammar<char> grammar = LoadGrammar("json.cgr");

        asmjit::StringLogger logger;

        MyErrorHandler errhandler;

        ParserEM64T<char> parser(grammar, &logger, &errhandler);

        size_t bank_size = 8 * 1024 * 1024;
        int bank_num = 8;

        IPCMaster ring_buffer(bank_size, bank_num);

        parser.set_buffer(ring_buffer.get_buffer());

        MemoryInput json("/home/ihara/Downloads/sf-city-lots-json-master/citylots.json");

        //Logger::WriteMessage(logger.getString());

        //char *json = LoadTextAligned("C:\\Users\\ihara\\Downloads\\sf-city-lots-json-master\\sf-city-lots-json-master\\citylots.json");

        //char *json = LoadTextAligned("C:\\Users\\ihara\\Downloads\\citylots.json");

        ParserRunner<ParserEM64T<char> > runner{ parser, json.get_buffer(), NULL };

        runner.run();

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
