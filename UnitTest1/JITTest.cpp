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

		TEST_METHOD(JITByteCompare1)
		{
			using namespace asmjit;

			JitRuntime rt;
			CodeHolder code;

			code.init(rt.getCodeInfo());

			X86Compiler cc(&code);

			cc.addFunc(FuncSignature1<int, void *>());

			X86Gp pReg = cc.newIntPtr();
			X86Gp aReg = cc.newGpd();

			cc.alloc(aReg, X86Gp::kIdBx);

			Assert::AreEqual((uint32_t)X86Gp::kIdAx, aReg.getKind());

			cc.setArg(0, pReg);

			cc.movzx(aReg, x86::byte_ptr(pReg));

			Label label = cc.newLabel();

			cc.sub(aReg, '0');
			cc.cmp(aReg, '9'-'0'+1);
			cc.jb(label);
			cc.mov(aReg, 0);
			cc.ret(aReg);
			cc.bind(label);
			cc.mov(aReg, 1);
			cc.ret(aReg);

			cc.endFunc();
			cc.finalize();

			int (*func)(char *ch);

			rt.add(&func, &code);

			char ch1 = '3', ch2 = '/', ch3 = ':';
			
			Assert::AreEqual(1, func(&ch1));
			Assert::AreEqual(0, func(&ch2));
			Assert::AreEqual(0, func(&ch3));
		}
	};
}
}
}