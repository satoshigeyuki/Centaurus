#include "stdafx.h"
#include "CppUnitTest.h"

#include "NFA.hpp"
#include "Grammar.hpp"
#include "CompositeATN.hpp"

#include "CATNLoader.hpp"

std::locale::id std::codecvt<char, char, std::mbstate_t>::id;
std::locale::id std::codecvt<char16_t, char, std::mbstate_t>::id;
std::locale::id std::codecvt<char32_t, char, std::mbstate_t>::id;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest1
{
	TEST_CLASS(CharClassTest)
    {
	public:
		TEST_METHOD(LoadGrammarFile)
		{
			// TODO: テスト コードをここに挿入します
			Centaurus::CompositeATN<char> catn = LoadCATN("../../../json.cgr");

			//Logger::WriteMessage("Hello World!!");
		}
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
	};
}