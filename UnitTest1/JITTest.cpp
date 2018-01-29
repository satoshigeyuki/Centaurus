#include "stdafx.h"
#include "asmjit/asmjit.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Microsoft
{
namespace VisualStudio
{
namespace CppUnitTestFramework
{
	TEST_CLASS(JITTest)
	{
		TEST_METHOD(JITAddFunc)
		{
			typedef int (*Func)(int a, int b);

			using namespace asmjit;

			JitRuntime rt;

			CodeHolder code;
			code.init(rt.getCodeInfo());

			X86Assembler as(&code);

			as.mov(x86::rax, x86::rcx);
			as.add(x86::rax, x86::rdx);
			as.ret();

			Func fn;

			Error err = rt.add(&fn, &code);

			if (err) Assert::Fail(L"Failed to compile the code.");

			int result = fn(100, 200);

			Assert::AreEqual(result, 300);

			rt.release(fn);
		}

		TEST_METHOD(JITBranchFunc1)
		{
			using namespace asmjit;

			typedef int (*Func)(void *ctrl, void *data);

			JitRuntime rt;

			CodeHolder code;
			code.init(rt.getCodeInfo());

			X86Assembler as(&code);

			as.vpbroadcastb(x86::xmm0, X86Mem(x86::rdx, 0));
			as.vmovdqa(x86::xmm1, X86Mem(x86::rcx, 0));
			as.vpcmpistri(x86::xmm1, x86::xmm0, 0x04);
			as.mov(x86::rax, x86::rcx);
			as.and_(x86::rcx, 0x07);
			as.ret();

			Func fn;
			Error err = rt.add(&fn, &code);

			if (err) Assert::Fail(L"Assembler reported an error.");

			char *data = "defg";
			alignas(16) char ctrl[] = "aabbccddeeffgghh";

			int result = fn(ctrl, data);

			Assert::AreEqual(6, result);
		}
	};
}
}
}