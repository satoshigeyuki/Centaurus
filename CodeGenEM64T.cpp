#include "dfa.hpp"
#include "atn.hpp"
#include "asmjit/asmjit.h"
#include "CodeGenEM64T.hpp"

namespace Centaurus
{
asmjit::Data128 pack_charclass(const CharClass<char>& cc)
{
    asmjit::Data128 d128;

    for (int i = 0; i < 8; i++)
    {
        if (cc[i].empty())
        {
            d128.ub[i * 2 + 0] = 0xFF;
            d128.ub[i * 2 + 1] = 0;
        }
        else
        {
            d128.ub[i * 2 + 0] = cc[i].start();
            d128.ub[i * 2 + 1] = cc[i].end() - 1;
        }
    }

    return d128;
}
asmjit::Data128 pack_charclass(const CharClass<wchar_t>& cc)
{
    asmjit::Data128 d128;

    for (int i = 0; i < 4; i++)
    {
        if (cc[i].empty())
        {
            d128.uw[i * 2 + 0] = 0xFFFF;
            d128.uw[i * 2 + 1] = 0;
        }
        else
        {
            d128.uw[i * 2 + 0] = cc[i].start();
            d128.uw[i * 2 + 1] = cc[i].end() - 1;
        }
    }

    return d128;
}

template<typename TCHAR>
ParserEM64T<TCHAR>::ParserEM64T(const Grammar<TCHAR>& grammar, asmjit::Logger *logger = NULL, asmjit::ErrorHandler *errhandler = NULL)
{
    m_code.init(m_runtime.getCodeInfo());
    if (logger != NULL)
        m_code.setLogger(logger);
    if (errhandler != NULL)
        m_code.setErrorHandler(errhandler);

    asmjit::X86Compiler cc(&m_code);
    std::unordered_map<Identifier, asmjit::CCFunc*> machine_map;

    asmjit::Label datalabel = cc.newLabel();
    asmjit::CBNode *datanode = cc.newDataNode(NULL, 64);

    asmjit::X86Mem astbuf_base(datalabel, 0);
    asmjit::X86Mem astbuf_head(datalabel, 8);

    cc.addFunc(asmjit::FuncSignature2<const void *, const void *, void *>(asmjit::CallConv::kIdHost));

    {
        asmjit::X86Gp inputreg = cc.newIntPtr();
        cc.setArg(0, inputreg);
        asmjit::X86Gp outputreg = cc.newIntPtr();
        cc.setArg(1, outputreg);

        cc.mov(astbuf_base, outputreg);
        cc.mov(astbuf_head, outputreg);

        for (const auto& p : grammar.get_machines())
        {
            machine_map.emplace(p.first, cc.newFunc(asmjit::FuncSignature1<const void *, const void *>(asmjit::CallConv::kIdHost)));
        }

        asmjit::CCFuncCall *root_call = cc.call(machine_map[grammar.get_root_id()]->getLabel(), asmjit::FuncSignature1<const void *, const void *>(asmjit::CallConv::kIdHost));
        root_call->setArg(0, inputreg);
        root_call->setRet(0, inputreg);

        cc.ret(inputreg);
        cc.endFunc();
    }

    CompositeATN<TCHAR> catn(grammar);

    for (const auto& p : grammar.get_machines())
    {
        cc.addFunc(machine_map[p.first]);

        emit_machine(cc, p.second, machine_map, catn, p.first, astbuf_base, astbuf_head);

        cc.endFunc();
    }

    cc.bind(datalabel);
    cc.addNode(datanode);

    cc.finalize();
}

template<typename TCHAR>
DryParserEM64T<TCHAR>::DryParserEM64T(const Grammar<TCHAR>& grammar, asmjit::Logger *logger = NULL)
{
    m_code.init(m_runtime.getCodeInfo());
    if (logger != NULL)
        m_code.setLogger(logger);

    asmjit::X86Compiler cc(&m_code);

    cc.addFunc(asmjit::FuncSignature1<const void *, const void *>(asmjit::CallConv::kIdHost));

    asmjit::X86Gp inputreg = cc.newIntPtr();
    cc.setArg(0, inputreg);
    
    std::unordered_map<Identifier, asmjit::CCFunc*> machine_map;

    for (const auto& p : grammar.get_machines())
    {
        machine_map.emplace(p.first, cc.newFunc(asmjit::FuncSignature1<const void *, const void *>(asmjit::CallConv::kIdHost)));
    }

    asmjit::CCFuncCall *root_call = cc.call(machine_map[grammar.get_root_id()]->getLabel(), asmjit::FuncSignature1<const void *, const void *>(asmjit::CallConv::kIdHost));
    root_call->setArg(0, inputreg);
    root_call->setRet(0, inputreg);

    cc.ret(inputreg);
    cc.endFunc();

    CompositeATN<TCHAR> catn(grammar);

    for (const auto& p : grammar.get_machines())
    {
        cc.addFunc(machine_map[p.first]);

        emit_machine(cc, p.second, machine_map, catn, p.first);

        cc.endFunc();
    }

    cc.finalize();
}

template<> CharClass<char> ParserEM64T<char>::m_skipfilter({ ' ', '\t', '\r', '\n' });
template<> CharClass<wchar_t> ParserEM64T<wchar_t>::m_skipfilter({ L' ', L'\t', L'\r', L'\n' });

template<> CharClass<char> DryParserEM64T<char>::m_skipfilter({' ', '\t', '\r', '\n'});
template<> CharClass<wchar_t> DryParserEM64T<wchar_t>::m_skipfilter({L' ', L'\t', L'\r', L'\n'});

template<typename TCHAR>
void ParserEM64T<TCHAR>::emit_machine(asmjit::X86Compiler& cc, const ATNMachine<TCHAR>& machine, std::unordered_map<Identifier, asmjit::CCFunc*>& machine_map, const CompositeATN<TCHAR>& catn, const Identifier& id, asmjit::X86Mem astbuf_base, asmjit::X86Mem astbuf_head)
{
    asmjit::X86Gp inputreg = cc.newIntPtr();
    cc.setArg(0, inputreg);

    std::vector<asmjit::Label> statelabels;

    for (int i = 0; i < machine.get_node_num(); i++)
    {
        statelabels.push_back(cc.newLabel());
    }

    asmjit::Label rejectlabel = cc.newLabel();

    {
        asmjit::X86Gp output_ptr_reg = cc.newIntPtr();
        cc.mov(output_ptr_reg, astbuf_head);
        asmjit::X86Gp output_base_reg = cc.newIntPtr();
        cc.mov(output_base_reg, astbuf_base);
        asmjit::X86Gp output_bound_reg = cc.newIntPtr();
        cc.mov(output_bound_reg, output_base_reg);
        cc.add(output_bound_reg, AST_BUF_SIZE);

        //Write the start marker to the AST buffer.
        //Structure of the start marker (128 bits, Little Endian):
        //128 127        112         80       64               0
        //+---+----------+-----------+--------+----------------+
        //| 1 | Reserved | Reserved2 | ATN ID | Start Position |
        //+---+----------+-----------+--------+----------------+

        cc.mov(asmjit::X86Mem(output_ptr_reg, 0), inputreg);

        // <=== Access Violation is handled here ===>

        cc.mov(asmjit::x86::qword_ptr(output_ptr_reg, 8), asmjit::Imm((uint64_t)machine.get_unique_id() | ((uint64_t)1 << 63)));
        cc.add(output_ptr_reg, 16);
        cc.cmp(output_ptr_reg, output_bound_reg);
        cc.cmovge(output_ptr_reg, output_base_reg);
        cc.mov(astbuf_head, output_ptr_reg);
    }

    for (int i = 0; i < machine.get_node_num(); i++)
    {
        cc.bind(statelabels[i]);

        const ATNNode<TCHAR>& node = machine.get_node(i);

        switch (node.type())
        {
        case ATNNodeType::Blank:
            break;
        case ATNNodeType::LiteralTerminal:
            MatchRoutineEM64T<TCHAR>::emit(cc, inputreg, rejectlabel, node.get_literal());
            break;
        case ATNNodeType::Nonterminal:
        {
            asmjit::CCFuncCall *nt_call = cc.call(machine_map[node.get_invoke()]->getLabel(), asmjit::FuncSignature1<const void *, const void *>(asmjit::CallConv::kIdHost));
            nt_call->setArg(0, inputreg);
            nt_call->setRet(0, inputreg);

            cc.cmp(inputreg, 0);
            cc.jz(rejectlabel);
        }
            break;
        case ATNNodeType::RegularTerminal:
            DFARoutineEM64T<TCHAR>::emit(cc, inputreg, rejectlabel, DFA<TCHAR>(node.get_nfa()));
            break;
        case ATNNodeType::WhiteSpace:
            SkipRoutineEM64T<TCHAR>::emit(cc, inputreg, m_skipfilter);
            break;
        }

        int outbound_num = node.get_transitions().size();
        if (outbound_num == 0)
        {
            //Write the end marker to the AST buffer.
            //Structure of the end marker:
            //128 127        112         80       64             0
            //+---+----------+-----------+--------+--------------+
            //| 0 | Reserved | Reserved2 | ATN ID | End Position |
            //+---+----------+-----------+--------+--------------+

            asmjit::X86Gp output_ptr_reg = cc.newIntPtr();
            cc.mov(output_ptr_reg, astbuf_head);
            asmjit::X86Gp output_base_reg = cc.newIntPtr();
            cc.mov(output_base_reg, astbuf_base);
            asmjit::X86Gp output_bound_reg = cc.newIntPtr();
            cc.mov(output_bound_reg, output_base_reg);
            cc.add(output_bound_reg, AST_BUF_SIZE);

            cc.mov(asmjit::X86Mem(output_ptr_reg, 0), inputreg);

            // <=== Access Violation is handled here ===>

            cc.mov(asmjit::x86::qword_ptr(output_ptr_reg, 8), asmjit::Imm(machine.get_unique_id()));
            cc.add(output_ptr_reg, 16);
            cc.cmp(output_ptr_reg, output_bound_reg);
            cc.cmove(output_ptr_reg, output_base_reg);
            cc.mov(astbuf_head, output_ptr_reg);

            cc.ret(inputreg);
        }
        else if (outbound_num == 1)
        {
            cc.jmp(statelabels[node.get_transition(0).dest()]);
        }
        else
        {
            std::vector<asmjit::Label> exitlabels;

            for (int j = 0; j < outbound_num; j++)
            {
                exitlabels.push_back(statelabels[node.get_transition(j).dest()]);
            }

            asmjit::X86Gp peekreg = cc.newIntPtr();
            cc.mov(peekreg, inputreg);

            LDFARoutineEM64T<TCHAR>::emit(cc, peekreg, rejectlabel, LookaheadDFA<TCHAR>(catn, catn.convert_atn_path(ATNPath(id, i))), exitlabels);
        }
    }

    cc.bind(rejectlabel);
    cc.mov(inputreg, 0);
    cc.ret(inputreg);
}

template<typename TCHAR>
void DryParserEM64T<TCHAR>::emit_machine(asmjit::X86Compiler& cc, const ATNMachine<TCHAR>& machine, std::unordered_map<Identifier, asmjit::CCFunc*>& machine_map, const CompositeATN<TCHAR>& catn, const Identifier& id)
{
    asmjit::X86Gp inputreg = cc.newIntPtr();
    cc.setArg(0, inputreg);

    std::vector<asmjit::Label> statelabels;

    for (int i = 0; i < machine.get_node_num(); i++)
    {
        statelabels.push_back(cc.newLabel());
    }

    asmjit::Label rejectlabel = cc.newLabel();

    for (int i = 0; i < machine.get_node_num(); i++)
    {
        cc.bind(statelabels[i]);

        const ATNNode<TCHAR>& node = machine.get_node(i);

        switch (node.type())
        {
        case ATNNodeType::Blank:
            break;
        case ATNNodeType::LiteralTerminal:
            MatchRoutineEM64T<TCHAR>::emit(cc, inputreg, rejectlabel, node.get_literal());
            break;
        case ATNNodeType::Nonterminal: {
            asmjit::CCFuncCall *nt_call = cc.call(machine_map[node.get_invoke()]->getLabel(), asmjit::FuncSignature1<const void *, const void *>(asmjit::CallConv::kIdHost));
            nt_call->setArg(0, inputreg);
            nt_call->setRet(0, inputreg);

            cc.cmp(inputreg, 0);
            cc.jz(rejectlabel);
            } break;
        case ATNNodeType::RegularTerminal:
            DFARoutineEM64T<TCHAR>::emit(cc, inputreg, rejectlabel, DFA<TCHAR>(node.get_nfa()));
            break;
        case ATNNodeType::WhiteSpace:
            SkipRoutineEM64T<TCHAR>::emit(cc, inputreg, m_skipfilter);
            break;
        }

        int outbound_num = node.get_transitions().size();
        if (outbound_num == 0)
        {
            cc.ret(inputreg);
        }
        else if (outbound_num == 1)
        {
            cc.jmp(statelabels[node.get_transition(0).dest()]);
        }
        else
        {
            std::vector<asmjit::Label> exitlabels;

            for (int j = 0; j < outbound_num; j++)
            {
                exitlabels.push_back(statelabels[node.get_transition(j).dest()]);
            }

            asmjit::X86Gp peekreg = cc.newIntPtr();
            cc.mov(peekreg, inputreg);

            LDFARoutineEM64T<TCHAR>::emit(cc, peekreg, rejectlabel, LookaheadDFA<TCHAR>(catn, catn.convert_atn_path(ATNPath(id, i))), exitlabels);
        }
    }

    cc.bind(rejectlabel);
    cc.mov(inputreg, 0);
    cc.ret(inputreg);
}

template<typename TCHAR>
DFARoutineEM64T<TCHAR>::DFARoutineEM64T(const DFA<TCHAR>& dfa, asmjit::Logger *logger)
{
    m_code.init(m_runtime.getCodeInfo());
    if (logger != NULL)
        m_code.setLogger(logger);

    asmjit::X86Compiler cc(&m_code);

    cc.addFunc(asmjit::FuncSignature1<const void *, const void *>(asmjit::CallConv::kIdHost));

    asmjit::X86Gp inputreg = cc.newIntPtr();
    cc.setArg(0, inputreg);
    asmjit::Label rejectlabel = cc.newLabel();

    emit(cc, inputreg, rejectlabel, dfa);

    cc.ret(inputreg);
    cc.endFunc();
    cc.finalize();
}

template<typename TCHAR>
void DFARoutineEM64T<TCHAR>::emit(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, asmjit::Label& rejectlabel, const DFA<TCHAR>& dfa)
{
    std::vector<asmjit::Label> statelabels;

    for (int i = 0; i < dfa.get_state_num(); i++)
    {
        statelabels.push_back(cc.newLabel());
    }

    asmjit::Label finishlabel = cc.newLabel();
    asmjit::X86Gp backupreg = cc.newIntPtr();

    cc.mov(backupreg, 0);

    for (int i = 0; i < dfa.get_state_num(); i++)
    {
        cc.bind(statelabels[i]);

        emit_state(cc, inputreg, backupreg, finishlabel, dfa[i], statelabels);
    }
    cc.bind(finishlabel);
    cc.mov(inputreg, backupreg);
}

template<>
void DFARoutineEM64T<char>::emit_state(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, asmjit::X86Gp& backupreg, asmjit::Label& rejectlabel, const DFAState<char>& state, std::vector<asmjit::Label>& labels)
{
    //The character is first read into a register from the stream
    asmjit::X86Gp cReg = cc.newGpz();
    //Another register is used for evacuating the character temporarily
	asmjit::X86Gp c2Reg = cc.newGpz();

    //Read the character from stream and advance the input position
	cc.movzx(cReg, asmjit::x86::byte_ptr(inputreg, 0));
	cc.mov(c2Reg, cReg);
	if (state.is_accept_state())
		cc.mov(backupreg, inputreg);
	cc.inc(inputreg);

	for (const auto& tr : state.get_transitions())
	{
		for (const auto& r : tr.label())
		{
			if (r.start() + 1 == r.end())
			{
                //The range consists of one character: test for equality and jump
				cc.cmp(cReg, r.start());
				cc.je(labels[tr.dest()]);
			}
			else
			{
                //The range consists of multiple characters: range check and jump
				cc.sub(cReg, r.start());
				cc.cmp(cReg, r.end() - r.start());
				cc.jb(labels[tr.dest()]);
				cc.mov(cReg, c2Reg);
			}
		}
	}
    //Jump to the "reject trampoline" and check if the input has ever been accepted
	cc.jmp(rejectlabel);
}

template<>
void DFARoutineEM64T<wchar_t>::emit_state(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, asmjit::X86Gp& backupreg, asmjit::Label& rejectlabel, const DFAState<wchar_t>& state, std::vector<asmjit::Label>& labels)
{
    //The character is first read into a register from the stream
    asmjit::X86Gp cReg = cc.newGpz();
    //Another register is used for evacuating the character temporarily
    asmjit::X86Gp c2Reg = cc.newGpz();

    //Read the character from stream and advance the input position
    cc.movzx(cReg, asmjit::x86::word_ptr(inputreg, 0));
    cc.mov(c2Reg, cReg);
    if (state.is_accept_state())
        cc.mov(backupreg, inputreg);
    cc.add(inputreg, 2);

    for (const auto& tr : state.get_transitions())
    {
        for (const auto& r : tr.label())
        {
            if (r.start() + 1 == r.end())
            {
                //The range consists of one character: test for equality and jump
                cc.cmp(cReg, r.start());
                cc.je(labels[tr.dest()]);
            }
            else
            {
                //The range consists of multiple characters: range check and jump
                cc.sub(cReg, r.start());
                cc.cmp(cReg, r.end() - r.start());
                cc.jb(labels[tr.dest()]);
                cc.mov(cReg, c2Reg);
            }
        }
    }
    //Jump to the "reject trampoline" and check if the input has ever been accepted
    cc.jmp(rejectlabel);
}

template<typename TCHAR>
LDFARoutineEM64T<TCHAR>::LDFARoutineEM64T(const LookaheadDFA<TCHAR>& ldfa, asmjit::Logger *logger)
{
    m_code.init(m_runtime.getCodeInfo());
    if (logger != NULL)
        m_code.setLogger(logger);

    asmjit::X86Compiler cc(&m_code);

    cc.addFunc(asmjit::FuncSignature1<int, const void *>(asmjit::CallConv::kIdHost));

    asmjit::X86Gp inputreg = cc.newIntPtr();
    cc.setArg(0, inputreg);
    asmjit::Label rejectlabel = cc.newLabel();

    std::vector<asmjit::Label> exitlabels;

    int decision_num = ldfa.get_color_num();
    for (int i = 0; i < decision_num; i++)
    {
        exitlabels.push_back(cc.newLabel());
    }

    emit(cc, inputreg, rejectlabel, ldfa, exitlabels);
    cc.jmp(rejectlabel);

    for (int i = 0; i < decision_num; i++)
    {
        cc.bind(exitlabels[i]);
        cc.mov(inputreg, asmjit::Imm(i + 1));
        cc.ret(inputreg);
    }

    cc.bind(rejectlabel);
    cc.mov(inputreg, 0);
    cc.ret(inputreg);
    
    cc.endFunc();
    cc.finalize();
}

template<typename TCHAR>
void LDFARoutineEM64T<TCHAR>::emit(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, asmjit::Label& rejectlabel, const LookaheadDFA<TCHAR>& ldfa, std::vector<asmjit::Label>& exitlabels)
{
    std::vector<asmjit::Label> statelabels;

    for (int i = 0; i < ldfa.get_state_num(); i++)
    {
        statelabels.push_back(cc.newLabel());
    }
    for (int i = 0; i < ldfa.get_state_num(); i++)
    {
        cc.bind(statelabels[i]);
        emit_state(cc, inputreg, rejectlabel, ldfa[i], statelabels, exitlabels);
    }
}

template<>
void LDFARoutineEM64T<char>::emit_state(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, asmjit::Label& rejectlabel, const LDFAState<char>& state, std::vector<asmjit::Label>& labels, std::vector<asmjit::Label>& exitlabels)
{
    asmjit::X86Gp cReg = cc.newGpz();
    asmjit::X86Gp c2Reg = cc.newGpz();

    cc.movzx(cReg, asmjit::x86::byte_ptr(inputreg, 0));
    cc.mov(c2Reg, cReg);
    cc.inc(inputreg);

    for (const auto& tr : state.get_transitions())
    {
        for (const auto& r : tr.label())
        {
            if (r.start() + 1 == r.end())
            {
                cc.cmp(cReg, r.start());
                if (tr.dest() >= 0)
                {
                    cc.je(labels[tr.dest()]);
                }
                else
                {
                    cc.je(exitlabels[-tr.dest() - 1]);
                }
            }
            else
            {
                cc.sub(cReg, r.start());
                cc.cmp(cReg, r.end() - r.start());
                if (tr.dest() >= 0)
                {
                    cc.jb(labels[tr.dest()]);
                }
                else
                {
                    cc.jb(exitlabels[-tr.dest() - 1]);
                }
                cc.mov(cReg, c2Reg);
            }
        }
    }
    cc.jmp(rejectlabel);
}

template<>
void LDFARoutineEM64T<wchar_t>::emit_state(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, asmjit::Label& rejectlabel, const LDFAState<wchar_t>& state, std::vector<asmjit::Label>& labels, std::vector<asmjit::Label>& exitlabels)
{
    //Nothing to note here. Just another specialization.

	asmjit::X86Gp cReg = cc.newGpz();
	asmjit::X86Gp c2Reg = cc.newGpz();

	cc.movzx(cReg, asmjit::x86::word_ptr(inputreg, 0));
	cc.mov(c2Reg, cReg);
	cc.add(inputreg, 2);

	for (const auto& tr : state.get_transitions())
	{
		for (const auto& r : tr.label())
		{
			if (r.start() + 1 == r.end())
			{
				cc.cmp(cReg, r.start());
				if (tr.dest() >= 0)
				{
					cc.je(labels[tr.dest()]);
				}
				else
				{
                    cc.je(exitlabels[-tr.dest() - 1]);
				}
			}
			else
			{
				cc.sub(cReg, r.start());
				cc.cmp(cReg, r.end() - r.start());
				if (tr.dest() >= 0)
				{
					cc.jb(labels[tr.dest()]);
				}
				else
				{
                    cc.jb(exitlabels[-tr.dest() - 1]);
				}
				cc.mov(cReg, c2Reg);
			}
		}
	}
    cc.jmp(rejectlabel);
}

template<typename TCHAR>
MatchRoutineEM64T<TCHAR>::MatchRoutineEM64T(const std::basic_string<TCHAR>& str, asmjit::Logger *logger)
{
    m_code.init(m_runtime.getCodeInfo());
    if (logger != NULL)
        m_code.setLogger(logger);

    asmjit::X86Compiler cc(&m_code);

    cc.addFunc(asmjit::FuncSignature2<const void *, const void *, void *>(asmjit::CallConv::kIdHost));

    asmjit::X86Gp inputreg = cc.newIntPtr();
    cc.setArg(0, inputreg);
    asmjit::Label rejectlabel = cc.newLabel();

    emit(cc, inputreg, rejectlabel, str);

    cc.ret(inputreg);

    cc.bind(rejectlabel);
    cc.mov(inputreg, 0);
    cc.ret(inputreg);
    
    cc.endFunc();
    cc.finalize();
}

template<>
void MatchRoutineEM64T<char>::emit(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, asmjit::Label& rejectlabel, const std::basic_string<char>& str)
{
	asmjit::X86Gp mireg = cc.newIntPtr();
    asmjit::X86Xmm loadreg = cc.newXmm();

	for (int i = 0; i < str.size(); i += 16)
	{
		int l1 = std::min(str.size() - i, (size_t)16);

		asmjit::Data128 d1;

		for (int j = 0; j < l1; j++)
		{
			d1.ub[j] = str[i + j];
		}
		
		cc.vmovdqu(loadreg, asmjit::X86Mem(inputreg, 0));
		cc.vpcmpistri(loadreg, cc.newXmmConst(asmjit::kConstScopeGlobal, d1), asmjit::Imm(0x18), mireg);
		cc.cmp(mireg, asmjit::Imm(l1));
		cc.jb(rejectlabel);
		cc.add(inputreg, l1);
	}
}
template<>
void MatchRoutineEM64T<wchar_t>::emit(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, asmjit::Label& rejectlabel, const std::basic_string<wchar_t>& str)
{
	asmjit::X86Gp mireg = cc.newIntPtr();
    asmjit::X86Xmm loadreg = cc.newXmm();

	for (int i = 0; i < str.size(); i += 8)
	{
		int l1 = std::min(str.size() - i, (size_t)8);

		asmjit::Data128 d1;

		for (int j = 0; j < l1; j++)
		{
			d1.uw[j] = str[i + j];
		}

		cc.vmovdqu(loadreg, asmjit::X86Mem(inputreg, 0));
		cc.vpcmpistri(loadreg, cc.newXmmConst(asmjit::kConstScopeGlobal, d1), asmjit::Imm(0x18), mireg);
		cc.cmp(mireg, asmjit::Imm(l1));
		cc.jb(rejectlabel);
		cc.add(inputreg, l1);
	}
}

template<typename TCHAR>
void SkipRoutineEM64T<TCHAR>::emit(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, const CharClass<TCHAR>& filter)
{
    asmjit::X86Xmm filterreg = cc.newXmm();
    asmjit::X86Mem filtermem = cc.newXmmConst(asmjit::kConstScopeGlobal, pack_charclass(filter));
    
    cc.vmovdqa(filterreg, filtermem);

    asmjit::Label looplabel = cc.newLabel();

    cc.bind(looplabel);

    emit_core(cc, inputreg, filterreg);

    cc.jg(looplabel);
}

template<>
void SkipRoutineEM64T<char>::emit_core(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, asmjit::X86Xmm& filterreg)
{
    asmjit::X86Xmm loadreg = cc.newXmm();
    asmjit::X86Gp mireg = cc.newGpz();

    cc.vmovdqu(loadreg, asmjit::X86Mem(inputreg, 0));
    cc.vpcmpistri(filterreg, loadreg, asmjit::Imm(0x14), mireg);
    cc.add(inputreg, mireg);
    cc.cmp(mireg, 15);
}

template<>
void SkipRoutineEM64T<wchar_t>::emit_core(asmjit::X86Compiler& cc, asmjit::X86Gp& inputreg, asmjit::X86Xmm& filterreg)
{
    asmjit::X86Xmm loadreg = cc.newXmm();
    asmjit::X86Gp mireg = cc.newGpz();

    cc.vmovdqu(loadreg, asmjit::X86Mem(inputreg, 0));
    cc.vpcmpistri(filterreg, loadreg, asmjit::Imm(0x15), mireg);
    cc.sal(mireg, 1);
    cc.add(inputreg, mireg);
    cc.cmp(mireg, 14);
}

template<typename TCHAR>
SkipRoutineEM64T<TCHAR>::SkipRoutineEM64T(const CharClass<TCHAR>& filter, asmjit::Logger *logger = NULL)
{
    m_code.init(m_runtime.getCodeInfo());
    if (logger != NULL)
        m_code.setLogger(logger);

    asmjit::X86Compiler cc(&m_code);

    cc.addFunc(asmjit::FuncSignature1<const void *, const void *>(asmjit::CallConv::kIdHost));

    asmjit::X86Gp inputreg = cc.newIntPtr();
    cc.setArg(0, inputreg);
    
    emit(cc, inputreg, filter);

    cc.ret(inputreg);
    cc.endFunc();
    cc.finalize();
}

template class ParserEM64T<char>;
template class ParserEM64T<wchar_t>;
template class DryParserEM64T<char>;
template class DryParserEM64T<wchar_t>;
template class DFARoutineEM64T<char>;
template class DFARoutineEM64T<wchar_t>;
template class LDFARoutineEM64T<char>;
template class LDFARoutineEM64T<wchar_t>;
template class MatchRoutineEM64T<char>;
template class MatchRoutineEM64T<wchar_t>;
template class SkipRoutineEM64T<char>;
template class SkipRoutineEM64T<wchar_t>;
}