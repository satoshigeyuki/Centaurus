﻿// PyCentaurus.cpp : DLL アプリケーション用にエクスポートされる関数を定義します。
//

#include "Grammar.hpp"
#include "Util.hpp"
#include "CodeGenEM64T.hpp"
#include "Stage1Runner.hpp"
#include "Stage2Runner.hpp"
#include "Stage3Runner.hpp"

#if defined(CENTAURUS_BUILD_WINDOWS)
#define CENTAURUS_EXPORT(T) extern "C" __declspec(dllexport) T __cdecl
#elif defined(CENTAURUS_BUILD_LINUX)
#define CENTAURUS_EXPORT(T) extern "C" T
#endif

namespace Centaurus __attribute__((visibility("default")))
{
	CENTAURUS_EXPORT(IGrammar *) GrammarCreate(const char *filename)
	{
		Stream grammar_stream(readwcsfromfile(filename));

		return new Grammar<char>(grammar_stream);
	}

	CENTAURUS_EXPORT(void) GrammarDestroy(IGrammar *grammar)
	{
		delete grammar;
	}

	CENTAURUS_EXPORT(void) GrammarPrint(IGrammar *grammar, const char *filename, int maxdepth)
	{
		std::wofstream ofs(filename);

		grammar->print(ofs, maxdepth);
	}

	CENTAURUS_EXPORT(void) GrammarEnumMachines(IGrammar *grammar, EnumMachinesCallback callback)
	{
		grammar->enum_machines(callback);
	}

	CENTAURUS_EXPORT(IParser *) ParserCreate(IGrammar *grammar, bool dry)
	{
		Grammar<char> *g_c = dynamic_cast<Grammar<char> *>(grammar);

		if (g_c)
		{
			if (dry)
				return new DryParserEM64T<char>(*g_c);
			else
				return new ParserEM64T<char>(*g_c);
		}

		return nullptr;
	}

	CENTAURUS_EXPORT(void) ParserDestroy(IParser *parser)
	{
		delete parser;
	}

	CENTAURUS_EXPORT(IChaser *) ChaserCreate(IGrammar *grammar)
	{
		Grammar<char> *g_c = dynamic_cast<Grammar<char> *>(grammar);

		if (g_c)
			return new ChaserEM64T<char>(*g_c);

		return nullptr;
	}

	CENTAURUS_EXPORT(void) ChaserDestroy(IChaser *chaser)
	{
		delete chaser;
	}

	CENTAURUS_EXPORT(Stage1Runner *) Stage1RunnerCreate(const char *filename, IParser *parser, size_t bank_size, int bank_num)
	{
		return new Stage1Runner(filename, parser, bank_size, bank_num);
	}

	CENTAURUS_EXPORT(void) RunnerStart(BaseRunner *runner)
	{
		runner->start();
	}

	CENTAURUS_EXPORT(void) RunnerWait(BaseRunner *runner)
	{
		runner->wait();
	}

	CENTAURUS_EXPORT(void) RunnerDestroy(BaseRunner *runner)
	{
		delete runner;
	}

	CENTAURUS_EXPORT(void) RunnerRegisterListener(BaseRunner *runner, ReductionListener listener, TransferListener xferlistener)
	{
		runner->register_listener(listener, xferlistener);
	}

	CENTAURUS_EXPORT(const void *) RunnerGetWindow(BaseRunner *runner)
	{
		return runner->get_input();
	}

	CENTAURUS_EXPORT(Stage2Runner *) Stage2RunnerCreate(const char *filename, IChaser *chaser, size_t bank_size, int bank_num, int master_pid)
	{
		return new Stage2Runner(filename, chaser, bank_size, bank_num, master_pid);
	}

	CENTAURUS_EXPORT(Stage3Runner *) Stage3RunnerCreate(const char *filename, IChaser *chaser, size_t bank_size, int bank_num, int master_pid)
	{
		return new Stage3Runner(filename, chaser, bank_size, bank_num, master_pid);
	}
}
