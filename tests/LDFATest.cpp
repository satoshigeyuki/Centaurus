#include "Grammar.hpp"
#include "LookaheadDFA.hpp"

#include "CATNLoader.hpp"
#include "loggerstream.hpp"

#if defined(CENTAURUS_BUILD_WINDOWS)
#include <Windows.h>
#endif

namespace Microsoft
{
namespace VisualStudio
{
namespace CppUnitTestFramework
{
TEST_CLASS(LDFATest)
{
	LoggerStreamBuf<char> m_narrowstreambuf;
public:
	TEST_METHOD_INITIALIZE(LDFATestInitialize)
	{
		//std::cout.rdbuf(&m_narrowstreambuf);
#if defined(CENTAURUS_BUILD_WINDOWS)
        SetCurrentDirectoryA(CENTAURUS_PROJECT_DIR);
#endif
	}
	TEST_METHOD(LDFAConstructionTest)
	{
        Logger::WriteMessage("Starting test.\n");

		Centaurus::CompositeATN<char> catn = LoadCATN("../../grammar/json.cgr");

        Logger::WriteMessage("CATN loaded.\n");

		Centaurus::LookaheadDFA<char> ldfa(catn, Centaurus::ATNPath(L"Object", 0));

		int lookup_result = ldfa.run("{}", 0, 0);

		Assert::AreEqual(4, lookup_result);
	}
};
}
}
}
