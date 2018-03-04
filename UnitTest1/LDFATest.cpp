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
	LoggerStreamBuf<char> m_narrowstreambuf;
public:
	TEST_METHOD_INITIALIZE(LDFATestInitialize)
	{
		std::cout.set_rdbuf(&m_narrowstreambuf);
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