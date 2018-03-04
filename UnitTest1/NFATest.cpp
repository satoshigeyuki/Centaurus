#include "stdafx.h"
#include "CppUnitTest.h"

#include "NFA.hpp"

using namespace Centaurus;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest1
{
	TEST_CLASS(NFATest)
	{
	public:
		TEST_METHOD(ConstructNFA1)
		{
			Stream stream(u"([a-f]ABCD)|([c-k]EFGH)/");
			NFA<char> nfa;

			nfa.parse(stream);

			Assert::IsTrue(nfa.run("aABCD"));
			Assert::IsTrue(nfa.run("dEFGH"));
			Assert::IsFalse(nfa.run("fABGH"));
		}
	};
}