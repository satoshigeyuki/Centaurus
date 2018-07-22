#include "stdafx.h"
#include "CppUnitTest.h"

#include "DFA.hpp"

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
                    Stream stream(L"(abc)+(def)?");

					NFA<char> nfa(stream);

					DFA<char> dfa(nfa);

					Assert::IsTrue(dfa.run("abcabcdef"));
				}
			};
		}
	}
}
