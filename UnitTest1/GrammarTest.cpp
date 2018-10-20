#include "stdafx.h"
#include "CppUnitTest.h"

#include "NFA.hpp"
#include "Grammar.hpp"
#include "CompositeATN.hpp"

#include "CATNLoader.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest1
{
TEST_CLASS(GrammarTest)
{
public:
	TEST_METHOD_INITIALIZE(InitGrammarTest)
	{
#if defined(CENTAURUS_BUILD_WINDOWS)
		SetCurrentDirectoryA(CENTAURUS_PROJECT_DIR);
#endif
	}
	TEST_METHOD(LoadGrammarFile)
	{
		// TODO: テスト コードをここに挿入します
		Centaurus::CompositeATN<char> catn = LoadCATN("grammar/json2.cgr");

		//Logger::WriteMessage("Hello World!!");

		
	}
	TEST_METHOD(GenerateATN)
	{
		Centaurus::Grammar<unsigned char> grammar = LoadGrammar<unsigned char>("grammar/json2.cgr");

		std::wofstream ofs("atn.dot");

		grammar.print(ofs, 3);
	}
};
}