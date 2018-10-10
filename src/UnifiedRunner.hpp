#pragma once

#if defined(CENTAURUS_BUILD_WINDOWS)
#include "stdafx.h"
#endif

#include "BaseRunner.hpp"
#include "CodeGenInterface.hpp"

namespace Centaurus
{
class UnifiedRunner
{
	IParser *m_parser;
private:
#if defined(CENTAURUS_BUILD_WINDOWS)
	static DWORD WINAPI thread_runner(LPVOID param)
#elif defined(CENTAURUS_BUILD_LINUX)
	static void *thread_runner(void *param)
#endif
	{
		UnifiedRunner *instance = reinterpret_cast<UnifiedRunner *>(param);

#if defined(CENTAURUS_BUILD_WINDOWS)
		ExitThread(0);
#elif defined(CENTAURUS_BUILD_LINUX)
		return NULL;
#endif
	}
public:
	UnifiedRunner(const char *filename, IParser *parser)
		: m_parser(parser)
	{

	}
};
}