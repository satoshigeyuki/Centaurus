// PyCentaurus.cpp : DLL アプリケーション用にエクスポートされる関数を定義します。
//

#include "stdafx.h"
#include "Grammar2.hpp"

#if defined(CENTAURUS_BUILD_WINDOWS)
#define CENTAURUS_EXPORT(T) extern "C" __declspec(dllexport) T __cdecl
#elif defined(CENTAURUS_BUILD_LINUX)
#define CENTAURUS_EXPORT(T) extern "C" T __attribute__((visibility("default")))
#endif

namespace Centaurus
{
	typedef GenericGrammar<wchar_t> *pyGrammar;

	CENTAURUS_EXPORT(pyGrammar) GrammarCreate()
	{
		return new GenericGrammar<wchar_t>();
	}

	CENTAURUS_EXPORT(void) GrammarDestroy(pyGrammar grammar)
	{
		delete grammar;
	}

	CENTAURUS_EXPORT(void) GrammarAddRule(pyGrammar grammar, const wchar_t *lhs, const wchar_t *rhs)
	{
		grammar->add_rule(lhs, rhs);
	}

	CENTAURUS_EXPORT(void) GrammarPrint(pyGrammar grammar, const char *filename, int maxdepth)
	{
		WideATNPrinter<wchar_t> printer(grammar->get_machines(), maxdepth);

		std::wofstream ofs(filename);

		printer.print(ofs, grammar->get_root_id());
	}
}