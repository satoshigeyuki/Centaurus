// PyCentaurus.cpp : DLL アプリケーション用にエクスポートされる関数を定義します。
//

#include "stdafx.h"
#include "Grammar.hpp"

#if defined(CENTAURUS_BUILD_WINDOWS)
#define CENTAURUS_EXPORT(T) extern "C" __declspec(dllexport) T __cdecl
#elif defined(CENTAURUS_BUILD_LINUX)
#define CENTAURUS_EXPORT(T) extern "C" T __attribute__((visibility("default")))
#endif

namespace Centaurus
{
	typedef Grammar<char> *pyGrammar;

	CENTAURUS_EXPORT(pyGrammar) GrammarCreate()
	{
		return new Grammar<char>();
	}

	CENTAURUS_EXPORT(void) GrammarDestroy(pyGrammar grammar)
	{
		delete grammar;
	}

	CENTAURUS_EXPORT(void) GrammarAddRule(pyGrammar grammar, const char *lhs, const char *rhs)
	{

	}
}