#include <iostream>
#include <sstream>
#include <fstream>
#include <locale>
#include <codecvt>
#include <vector>
#include <string>

#include "clipp.h"
#include "Grammar.hpp"
#include "CodeGenEM64T.hpp"
#include "Stage1Runner.hpp"
#include "Stage2Runner.hpp"
#include "Stage3Runner.hpp"
#include "Encoding.hpp"
#include "Util.hpp"

#if defined(CENTAURUS_BUILD_LINUX)
#include <sys/time.h>
#elif defined(CENTAURUS_BUILD_WINDOWS)
#include <Windows.h>
#endif

/*
 * Workaround for a bug in Visual C++ standard library
 */
std::locale::id std::codecvt<char, char, std::mbstate_t>::id;
std::locale::id std::codecvt<char16_t, char, std::mbstate_t>::id;
std::locale::id std::codecvt<char32_t, char, std::mbstate_t>::id;
//std::locale::id std::codecvt<wchar_t, char, std::mbstate_t>::id;

using namespace Centaurus;

static IGrammar *LoadGrammar(const char *filename)
{
    std::wifstream grammar_file(filename, std::ios::in);

    std::wstring grammar_str(std::istreambuf_iterator<wchar_t>(grammar_file), {});

    Stream stream(std::move(grammar_str));

    Grammar<unsigned char> *grammar = new Grammar<unsigned char>();

    grammar->parse(stream);

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

int main(int argc, char *argv[])
{
    std::wcin.imbue(std::locale(""));
    std::wcout.imbue(std::locale(""));

    Encoder enc;

	enum
	{
		GenerateATN,
        GenerateCATN,
		GenerateNFA,
		GenerateLDFA,
		GenerateDFA
	} mode;
    std::string output_path, grammar_path, atn_path, machine_name;
    int max_depth = 3;
	bool help_flag = false, optimize_flag = false;

	clipp::group cli =
    (
		clipp::one_of
        (
            clipp::in_sequence
            (
                clipp::command("generate-atn").set(mode, GenerateATN),
                (
                    clipp::value("grammar file", grammar_path).required(true),
                    clipp::in_sequence
                    (
                        clipp::option("-d", "--max-depth"),
                        clipp::value("maxdepth", max_depth)
                    ),
                    clipp::option("--optimize").set(optimize_flag, true)
                )
            ),
            clipp::in_sequence
            (
                clipp::command("generate-catn").set(mode, GenerateCATN),
                (
                    clipp::value("grammar file", grammar_path).required(true),
                    clipp::in_sequence
                    (
                        clipp::option("-m", "--machine").required(true),
                        clipp::value("machine name", machine_name)
                    )
                )
            ),
		    clipp::in_sequence
            (
                clipp::command("generate-nfa").set(mode, GenerateNFA),
                (
                    clipp::value("grammar file", grammar_path).required(true),
                    clipp::in_sequence
                    (
                        clipp::option("-p", "--path").required(true),
                        clipp::value("ATN path", atn_path)
                    ).doc("ATN Path to the target object")
                )
            ),
		    clipp::in_sequence
            (
                clipp::command("generate-ldfa").set(mode, GenerateLDFA),
                (
                    clipp::value("grammar file", grammar_path).required(true),
                    clipp::in_sequence
                    (
                        clipp::option("-p", "--path").required(true),
                        clipp::value("ATN path", atn_path)
                    )
                )
            ),
		    clipp::in_sequence
            (
                clipp::command("generate-dfa").set(mode, GenerateDFA),
                (
                    clipp::value("grammar file", grammar_path).required(true),
                    clipp::in_sequence
                    (
                        clipp::option("-p", "--path").required(true),
                        clipp::value("ATN path", atn_path)
                    ),
                    clipp::option("--optimize").set(optimize_flag, true)
                )
            )
        ),
		clipp::option("-h", "--help").doc("Display this help message").set(help_flag),
        clipp::in_sequence
        (
            clipp::option("-f", "--outfile"),
            clipp::value("outfile", output_path)
        ).doc("Output filename (defaults to stdout")
	);

	if (clipp::parse(argc, argv, cli))
	{
		if (help_flag)
		{
			std::cerr << clipp::make_man_page(cli, argv[0]);
			return 0;
		}

        std::unique_ptr<IGrammar> grammar(LoadGrammar(grammar_path.c_str()));

        std::wofstream output_file;
        if (!output_path.empty())
        {
            output_file.open(output_path.c_str());
        }
        std::wostream& output_stream = output_path.empty() ? std::wcout : output_file;

		switch (mode)
		{
		case GenerateATN:
            grammar->print_grammar(output_stream, max_depth, optimize_flag);
			return 0;
        case GenerateCATN:
            grammar->print_catn(output_stream, Identifier(enc.mbstowcs(machine_name)));
            return 0;
        case GenerateNFA:
            grammar->print_nfa(output_stream, ATNPath(enc.mbstowcs(atn_path)));
            return 0;
        case GenerateLDFA:
            grammar->print_ldfa(output_stream, ATNPath(enc.mbstowcs(atn_path)));
            return 0;
        case GenerateDFA:
            grammar->print_dfa(output_stream, ATNPath(enc.mbstowcs(atn_path)), optimize_flag);
            return 0;
		}
	}
	else
	{
		std::cerr << clipp::make_man_page(cli, argv[0]);
		return -1;
	}
    return 0;
}
