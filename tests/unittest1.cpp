#include "CppUnitTest.h"

#include "NFA.hpp"
#include "Grammar.hpp"
#include "CompositeATN.hpp"

#include "CATNLoader.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest1
{
	TEST_CLASS(CharClassTest)
    {
	public:
		TEST_METHOD(DiffAndInt)
		{
			using namespace Centaurus;

			CharClass<char> cc1, cc2;

			cc1.append(Range<char>('c', 'f'));
			cc1.append(Range<char>('r', 'w'));
			cc2.append(Range<char>('a', 'e'));
			cc2.append(Range<char>('f', 'k'));
			cc2.append(Range<char>('m', 't'));

			std::array<CharClass<char>, 3> dai = cc1.diff_and_int(cc2);

			CharClass<char> diff1, diff2, inter;

			diff1.append(Range<char>('e', 'f'));
			diff1.append(Range<char>('t', 'w'));
			diff2.append(Range<char>('a', 'c'));
			diff2.append(Range<char>('f', 'k'));
			diff2.append(Range<char>('m', 'r'));
			inter.append(Range<char>('c', 'e'));
			inter.append(Range<char>('r', 't'));

			Logger::WriteMessage(ToString(dai[0]).c_str());
			Logger::WriteMessage(ToString(diff1).c_str());
			Logger::WriteMessage(ToString(dai[1]).c_str());
			Logger::WriteMessage(ToString(diff2).c_str());
			Logger::WriteMessage(ToString(dai[2]).c_str());
			Logger::WriteMessage(ToString(inter).c_str());

			Assert::AreEqual(diff1, dai[0]);
			Assert::AreEqual(diff2, dai[1]);
			Assert::AreEqual(inter, dai[2]);
		}
		TEST_METHOD(InvertCC)
		{
			using namespace Centaurus;

			Stream src_stream(L"^\\\\\"]");

			CharClass<char> cc1(src_stream);

			CharClass<char> cc1_ref;

			cc1_ref.append(Range<char>('\0', '"'));
			cc1_ref.append(Range<char>('#', '\\'));
			cc1_ref.append(Range<char>(']', '\x7F'));

			Assert::AreEqual(cc1_ref, cc1);
		}
		TEST_METHOD(InvertCC2)
		{
			using namespace Centaurus;

			Stream src_stream(L"^\\\\\"]");
			Stream src_stream_ref(L"\\\\\"]");

			CharClass<char> cc1(src_stream);
			CharClass<char> cc1_ref(src_stream_ref);

			Logger::WriteMessage(Microsoft::VisualStudio::CppUnitTestFramework::ToString(cc1).c_str());
			
			Assert::AreEqual(cc1_ref, ~cc1);
		}
	};
}
