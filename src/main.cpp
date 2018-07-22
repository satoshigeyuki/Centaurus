#include <iostream>
#include <sstream>
#include <fstream>
#include <locale>
#include <codecvt>

#include "Grammar.hpp"
#include "CodeGenEM64T.hpp"
#include "Stage1Runner.hpp"
#include "Stage2Runner.hpp"
#include "Stage3Runner.hpp"

#if defined(CENTAURUS_BUILD_LINUX)
#include <sys/time.h>
#elif defined(CENTAURUS_BUILD_WINDOWS)
#include <Windows.h>
#endif

std::locale::id std::codecvt<char, char, std::mbstate_t>::id;
std::locale::id std::codecvt<char16_t, char, std::mbstate_t>::id;
std::locale::id std::codecvt<char32_t, char, std::mbstate_t>::id;
//std::locale::id std::codecvt<wchar_t, char, std::mbstate_t>::id;

using namespace Centaurus;

static Grammar<char> LoadGrammar(const char *filename)
{
    std::wifstream grammar_file(filename, std::ios::in);

    std::wstring grammar_str(std::istreambuf_iterator<wchar_t>(grammar_file), {});

    Stream stream(std::move(grammar_str));

    Grammar<char> grammar;

    try
    {
        grammar.parse(stream);
    }
    catch (const std::exception& ex)
    {
        //Assert::Fail(L"ATN construction failed.");
    }

    return grammar;
}

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

int main(int argc, const char *argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: centaurus [GrammarFile] [Nthreads]" << std::endl;
        return -1;
    }

    Grammar<char> grammar = LoadGrammar(argv[1]);

    asmjit::StringLogger logger;

    ParserEM64T<char> parser(grammar, &logger, NULL);
    ChaserEM64T<char> chaser(grammar);

#if defined(CENTAURUS_BUILD_WINDOWS)
    const char *input_path = "C:\\Users\\ihara\\Downloads\\sf-city-lots-json-master\\sf-city-lots-json-master\\citylots.json";
#elif defined(CENTAURUS_BUILD_LINUX)
    //const char *input_path = "/mnt/c/Users/ihara/Downloads/sf-city-lots-json-master/sf-city-lots-json-master/citylots.json";
    const char *input_path = "/home/ihara/ramdisk/citylots.json";
#endif

    Stage1Runner<ParserEM64T<char> > runner1{ input_path, parser, 8 * 1024 * 1024, 16 };
#if defined(CENTAURUS_BUILD_WINDOWS)
    int pid = GetCurrentProcessId();
#elif defined(CENTAURUS_BUILD_LINUX)
    int pid = getpid();
#endif
    std::vector<Stage2Runner<ChaserEM64T<char> > *> st2_runners;
    for (int i = 0; i < atoi(argv[2]); i++)
    {
        st2_runners.push_back(new Stage2Runner<ChaserEM64T<char> >{input_path, chaser, 8 * 1024 * 1024, 16, pid});
    }
    Stage3Runner<ChaserEM64T<char> > runner3{ input_path, chaser, 8 * 1024 * 1024, 16, pid };

    uint64_t start_clock = get_us_clock();

    runner1.start();
    /*for (auto p : st2_runners)
    {
        p->start();
    }
    runner3.start();*/

    runner1.wait();
    /*for (auto p : st2_runners)
    {
        p->wait();
    }
    runner3.wait();*/

    uint64_t end_clock = get_us_clock();

    double elapsed_ms = (double)(end_clock - start_clock) * 1E-3;

    std::cout << "Elapsed time: " << elapsed_ms << "[ms]" << std::endl;

    return 0;
}
