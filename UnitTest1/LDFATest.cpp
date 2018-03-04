#include "stdafx.h"
#include "Grammar.hpp"
#include "LookaheadDFA.hpp"

#include "CATNLoader.hpp"
#include "loggerstream.hpp"

namespace Microsoft
{
namespace VisualStudio
{
namespace CppUnitTestFramework
{
TEST_CLASS(LDFATest)
{
public:
	TEST_METHOD_INITIALIZE(LDFATestInitialize)
	{
        static LoggerStreamBuf<char> m_narrowstreambuf;
		std::cout.rdbuf(&m_narrowstreambuf);
	}
	TEST_METHOD(LDFAConstructionTest)
	{
		Centaurus::CompositeATN<char> catn = LoadCATN("../../../json.cgr");

		Centaurus::LookaheadDFA<char> ldfa(catn, Centaurus::ATNPath(u"Object", 0));

		int lookup_result = ldfa.run("{}", 0, 0);

		Assert::AreEqual(4, lookup_result);
	}
};
}
}
}
