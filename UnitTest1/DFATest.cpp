#include "stdafx.h"
#include "CppUnitTest.h"

#include "dfa.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Centaurus;

namespace Microsoft
{
	namespace VisualStudio
	{
		namespace CppUnitTestFramework
		{
			TEST_CLASS(DFATest)
			{
			public:
				TEST_METHOD(ConstructDFA1)
				{
					NFA<char> nfa(Stream(u"(abc)+(def)?"));

					DFA<char> dfa(nfa);

					Assert::IsTrue(dfa.run("abcabcdef"));
				}
			};
		}
	}
}