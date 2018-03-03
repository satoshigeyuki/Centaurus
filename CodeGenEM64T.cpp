#include "dfa.hpp"
#include "atn.hpp"
#include "asmjit/asmjit.h"
#include "CodeGenEM64T.hpp"

asmjit::X86Gp CONTEXT_REG = asmjit::x86::r8;
asmjit::X86Gp INPUT_REG = asmjit::x86::rsi;
asmjit::X86Gp OUTPUT_REG = asmjit::x86::rdi;
asmjit::X86Gp OUTPUT_BOUND_REG = asmjit::x86::r9;
asmjit::X86Gp BACKUP_REG = asmjit::x86::rbx;
asmjit::X86Gp PEEK_REG = asmjit::x86::rbx;
asmjit::X86Gp CHAR_REG = asmjit::x86::rax;
asmjit::X86Gp CHAR2_REG = asmjit::x86::rcx;

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

static void emit_parser_prolog(asmjit::X86Assembler& as)
{
    as.push(asmjit::x86::r9);
    as.push(asmjit::x86::r8);
    as.push(asmjit::x86::rbp);
    as.push(asmjit::x86::rdi);
    as.push(asmjit::x86::rsi);
    as.push(asmjit::x86::rdx);
    as.push(asmjit::x86::rcx);
    as.push(asmjit::x86::rbx);
    as.push(asmjit::x86::rax);

    as.mov(CONTEXT_REG, asmjit::X86Mem(asmjit::x86::rsp, 16));
    as.mov(INPUT_REG, asmjit::X86Mem(asmjit::x86::rsp, 24));
    as.mov(OUTPUT_REG, asmjit::X86Mem(asmjit::x86::rsp, 56));
}

static void emit_parser_epilog(asmjit::X86Assembler& as)
{
    as.mov(asmjit::x86::rbx, asmjit::x86::rax);
    as.pop(asmjit::x86::rax);
    as.mov(asmjit::x86::rax, asmjit::x86::rbx);

    as.pop(asmjit::x86::rbx);
    as.pop(asmjit::x86::rcx);
    as.pop(asmjit::x86::rdx);
    as.pop(asmjit::x86::rsi);
    as.pop(asmjit::x86::rdi);
    as.pop(asmjit::x86::rbp);
    as.pop(asmjit::x86::r8);
    as.pop(asmjit::x86::r9);

    as.ret();
}

template<typename TCHAR>
ParserEM64T<TCHAR>::ParserEM64T(const Grammar<TCHAR>& grammar, asmjit::Logger *logger = NULL, asmjit::ErrorHandler *errhandler = NULL)
{
    m_code.init(m_runtime.getCodeInfo());
    if (logger != NULL)
        m_code.setLogger(logger);
    if (errhandler != NULL)
        m_code.setErrorHandler(errhandler);

    asmjit::X86Assembler as(&m_code);

    std::unordered_map<Identifier, asmjit::Label> machine_map;

    emit_parser_prolog(as);

    as.mov(OUTPUT_BOUND_REG, OUTPUT_REG);
    as.add(OUTPUT_BOUND_REG, asmjit::Imm(AST_BUF_SIZE));

    {
        for (const auto& p : grammar.get_machines())
        {
            machine_map.emplace(p.first, as.newLabel());
        }

        as.call(machine_map[grammar.get_root_id()]);
    }

    emit_parser_epilog(as);

    CompositeATN<TCHAR> catn(grammar);

    for (const auto& p : grammar.get_machines())
    {
        as.bind(machine_map[p.first]);

        emit_machine(as, p.second, machine_map, catn, p.first);
    }

    as.finalize();

    m_runtime.add(&m_func, &m_code);
}

template<typename TCHAR>
DryParserEM64T<TCHAR>::DryParserEM64T(const Grammar<TCHAR>& grammar, asmjit::Logger *logger = NULL)
{
    m_code.init(m_runtime.getCodeInfo());
    if (logger != NULL)
        m_code.setLogger(logger);

    asmjit::X86Assembler as(&m_code);

    std::unordered_map<Identifier, asmjit::Label> machine_map;

    emit_parser_prolog(as);

    {
        for (const auto& p : grammar.get_machines())
        {
            machine_map.emplace(p.first, as.newLabel());
        }

        as.call(machine_map[grammar.get_root_id()]);
    }

    emit_parser_epilog(as);

    CompositeATN<TCHAR> catn(grammar);

    for (const auto& p : grammar.get_machines())
    {
        as.bind(machine_map[p.first]);

        emit_machine(as, p.second, machine_map, catn, p.first);
    }

    as.finalize();
}

template<> CharClass<char> ParserEM64T<char>::m_skipfilter({ ' ', '\t', '\r', '\n' });
template<> CharClass<wchar_t> ParserEM64T<wchar_t>::m_skipfilter({ L' ', L'\t', L'\r', L'\n' });

template<> CharClass<char> DryParserEM64T<char>::m_skipfilter({' ', '\t', '\r', '\n'});
template<> CharClass<wchar_t> DryParserEM64T<wchar_t>::m_skipfilter({L' ', L'\t', L'\r', L'\n'});

template<typename TCHAR>
void ParserEM64T<TCHAR>::emit_machine(asmjit::X86Assembler& as, const ATNMachine<TCHAR>& machine, std::unordered_map<Identifier, asmjit::Label>& machine_map, const CompositeATN<TCHAR>& catn, const Identifier& id)
{
    std::vector<asmjit::Label> statelabels;

    for (int i = 0; i < machine.get_node_num(); i++)
    {
        statelabels.push_back(as.newLabel());
    }

    asmjit::Label rejectlabel = as.newLabel();
    asmjit::Label requestpage1_label = as.newLabel();
    asmjit::Label requestpage2_label = as.newLabel();

    {
        //Write the start marker to the AST buffer.
        //Structure of the start marker (128 bits, Little Endian):
        //128        80  79       64               0
        //+----------+---+--------+----------------+
        //| Reserved | 1 | ATN ID | Start Position |
        //+----------+---+--------+----------------+

        as.mov(asmjit::X86Mem(OUTPUT_REG, 0), INPUT_REG);
        as.mov(asmjit::x86::word_ptr(OUTPUT_REG, 8), asmjit::Imm((uint64_t)machine.get_unique_id() | ((uint64_t)1 << 15)));
        as.add(OUTPUT_REG, 16);
        as.cmp(OUTPUT_REG, OUTPUT_BOUND_REG);
        as.je(requestpage1_label);
    }

    for (int i = 0; i < machine.get_node_num(); i++)
    {
        as.bind(statelabels[i]);

        const ATNNode<TCHAR>& node = machine.get_node(i);

        switch (node.type())
        {
        case ATNNodeType::Blank:
            break;
        case ATNNodeType::LiteralTerminal:
            MatchRoutineEM64T<TCHAR>::emit(as, rejectlabel, node.get_literal());
            break;
        case ATNNodeType::Nonterminal:
        {
            as.call(machine_map[node.get_invoke()]);
            as.cmp(inputreg, 0);
            as.jz(rejectlabel);
        }
            break;
        case ATNNodeType::RegularTerminal:
            DFARoutineEM64T<TCHAR>::emit(as, rejectlabel, DFA<TCHAR>(node.get_nfa()));
            break;
        case ATNNodeType::WhiteSpace:
            SkipRoutineEM64T<TCHAR>::emit(as, m_skipfilter);
            break;
        }

        int outbound_num = node.get_transitions().size();
        if (outbound_num == 0)
        {
            //Write the end marker to the AST buffer.
            //Structure of the end marker:
            //128        80  79       64             0
            //+----------+---+--------+--------------+
            //| Reserved | 0 | ATN ID | End Position |
            //+----------+---+--------+--------------+

            as.mov(asmjit::X86Mem(OUTPUT_REG, 0), INPUT_REG);
            as.mov(asmjit::x86::word_ptr(OUTPUT_REG, 8), asmjit::Imm(machine.get_unique_id()));
            as.add(OUTPUT_REG, 16);
            as.cmp(OUTPUT_REG, OUTPUT_BOUND_REG);
            as.je(requestpage2_label);
            as.ret();
        }
        else if (outbound_num == 1)
        {
            as.jmp(statelabels[node.get_transition(0).dest()]);
        }
        else
        {
            std::vector<asmjit::Label> exitlabels;

            for (int j = 0; j < outbound_num; j++)
            {
                exitlabels.push_back(statelabels[node.get_transition(j).dest()]);
            }

            LDFARoutineEM64T<TCHAR>::emit(as, rejectlabel, LookaheadDFA<TCHAR>(catn, catn.convert_atn_path(ATNPath(id, i))), exitlabels);
        }
    }

    as.bind(rejectlabel);
    as.mov(inputreg, 0);
    as.ret();

    as.bind(requestpage1_label);
    {
        as.jmp(statelabels[0]);
    }
    as.bind(requestpage2_label);
    {
        as.ret();
    }
}

template<typename TCHAR>
void * __cdecl ParserEM64T<TCHAR>::request_page(void *context)
{
    ParserEM64T<TCHAR> *parser = (ParserEM64T<TCHAR> *)context;

    parser->m_flipcount++;

    return parser->m_buffer;
}

template<typename TCHAR>
void DryParserEM64T<TCHAR>::emit_machine(asmjit::X86Assembler& as, const ATNMachine<TCHAR>& machine, std::unordered_map<Identifier, asmjit::Label>& machine_map, const CompositeATN<TCHAR>& catn, const Identifier& id)
{
    std::vector<asmjit::Label> statelabels;

    for (int i = 0; i < machine.get_node_num(); i++)
    {
        statelabels.push_back(as.newLabel());
    }

    asmjit::Label rejectlabel = as.newLabel();

    for (int i = 0; i < machine.get_node_num(); i++)
    {
        as.bind(statelabels[i]);

        const ATNNode<TCHAR>& node = machine.get_node(i);

        switch (node.type())
        {
        case ATNNodeType::Blank:
            break;
        case ATNNodeType::LiteralTerminal:
            MatchRoutineEM64T<TCHAR>::emit(as, rejectlabel, node.get_literal());
            break;
        case ATNNodeType::Nonterminal: {
            } break;
        case ATNNodeType::RegularTerminal:
            DFARoutineEM64T<TCHAR>::emit(as, rejectlabel, DFA<TCHAR>(node.get_nfa()));
            break;
        case ATNNodeType::WhiteSpace:
            SkipRoutineEM64T<TCHAR>::emit(as, m_skipfilter);
            break;
        }

        int outbound_num = node.get_transitions().size();
        if (outbound_num == 0)
        {
            as.ret();
        }
        else if (outbound_num == 1)
        {
            as.jmp(statelabels[node.get_transition(0).dest()]);
        }
        else
        {
            std::vector<asmjit::Label> exitlabels;

            for (int j = 0; j < outbound_num; j++)
            {
                exitlabels.push_back(statelabels[node.get_transition(j).dest()]);
            }

            LDFARoutineEM64T<TCHAR>::emit(as, rejectlabel, LookaheadDFA<TCHAR>(catn, catn.convert_atn_path(ATNPath(id, i))), exitlabels);
        }
    }

    as.bind(rejectlabel);
    as.mov(inputreg, 0);
    as.ret();
}

template<typename TCHAR>
DFARoutineEM64T<TCHAR>::DFARoutineEM64T(const DFA<TCHAR>& dfa, asmjit::Logger *logger)
{
    m_code.init(m_runtime.getCodeInfo());
    if (logger != NULL)
        m_code.setLogger(logger);

    asmjit::X86Assembler as(&m_code);
    emit_parser_prolog(as);

    asmjit::Label rejectlabel = as.newLabel();

    emit(as, rejectlabel, dfa);

    emit_parser_epilog(as);
    as.finalize();
}

template<typename TCHAR>
void DFARoutineEM64T<TCHAR>::emit(asmjit::X86Assembler& as, asmjit::Label& rejectlabel, const DFA<TCHAR>& dfa)
{
    std::vector<asmjit::Label> statelabels;

    for (int i = 0; i < dfa.get_state_num(); i++)
    {
        statelabels.push_back(cc.newLabel());
    }

    asmjit::Label finishlabel = as.newLabel();

    as.mov(BACKUP_REG, 0);

    for (int i = 0; i < dfa.get_state_num(); i++)
    {
        as.bind(statelabels[i]);

        emit_state(as, finishlabel, dfa[i], statelabels);
    }
    as.bind(finishlabel);
    as.mov(INPUT_REG, BACKUP_REG);
}

template<>
void DFARoutineEM64T<char>::emit_state(asmjit::X86Assembler& as, asmjit::Label& rejectlabel, const DFAState<char>& state, std::vector<asmjit::Label>& labels)
{
    //Read the character from stream and advance the input position
	as.movzx(CHAR_REG, asmjit::x86::byte_ptr(INPUT_REG, 0));
	as.mov(CHAR2_REG, CHAR_REG);
	if (state.is_accept_state())
		as.mov(BACKUP_REG, INPUT_REG);
	as.inc(INPUT_REG);

	for (const auto& tr : state.get_transitions())
	{
		for (const auto& r : tr.label())
		{
			if (r.start() + 1 == r.end())
			{
                //The range consists of one character: test for equality and jump
				as.cmp(CHAR_REG, r.start());
				as.je(labels[tr.dest()]);
			}
			else
			{
                //The range consists of multiple characters: range check and jump
				as.sub(CHAR_REG, r.start());
				as.cmp(CHAR_REG, r.end() - r.start());
				as.jb(labels[tr.dest()]);
				as.mov(CHAR_REG, CHAR2_REG);
			}
		}
	}
    //Jump to the "reject trampoline" and check if the input has ever been accepted
	as.jmp(rejectlabel);
}

template<>
void DFARoutineEM64T<wchar_t>::emit_state(asmjit::X86Assembler& as, asmjit::Label& rejectlabel, const DFAState<wchar_t>& state, std::vector<asmjit::Label>& labels)
{
    //Read the character from stream and advance the input position
    as.movzx(CHAR_REG, asmjit::x86::word_ptr(INPUT_REG, 0));
    as.mov(CHAR2_REG, CHAR_REG);
    if (state.is_accept_state())
        as.mov(BACKUP_REG, INPUT_REG);
    as.add(INPUT_REG, 2);

    for (const auto& tr : state.get_transitions())
    {
        for (const auto& r : tr.label())
        {
            if (r.start() + 1 == r.end())
            {
                //The range consists of one character: test for equality and jump
                as.cmp(CHAR_REG, r.start());
                as.je(labels[tr.dest()]);
            }
            else
            {
                //The range consists of multiple characters: range check and jump
                as.sub(CHAR_REG, r.start());
                as.cmp(CHAR_REG, r.end() - r.start());
                as.jb(labels[tr.dest()]);
                as.mov(CHAR_REG, CHAR2_REG);
            }
        }
    }
    //Jump to the "reject trampoline" and check if the input has ever been accepted
    as.jmp(rejectlabel);
}

template<typename TCHAR>
LDFARoutineEM64T<TCHAR>::LDFARoutineEM64T(const LookaheadDFA<TCHAR>& ldfa, asmjit::Logger *logger)
{
    m_code.init(m_runtime.getCodeInfo());
    if (logger != NULL)
        m_code.setLogger(logger);

    asmjit::X86Assembler as(&m_code);

    emit_parser_prolog(as);

    asmjit::Label rejectlabel = as.newLabel();

    std::vector<asmjit::Label> exitlabels;

    int decision_num = ldfa.get_color_num();
    for (int i = 0; i < decision_num; i++)
    {
        exitlabels.push_back(as.newLabel());
    }

    emit(as, rejectlabel, ldfa, exitlabels);
    as.jmp(rejectlabel);

    for (int i = 0; i < decision_num; i++)
    {
        as.bind(exitlabels[i]);
        as.mov(INPUT_REG, asmjit::Imm(i + 1));
        as.ret();
    }

    as.bind(rejectlabel);
    as.mov(INPUT_REG, 0);

    emit_parser_epilog(as);
   
    as.finalize();
}

template<typename TCHAR>
void LDFARoutineEM64T<TCHAR>::emit(asmjit::X86Compiler& cc, asmjit::Label& rejectlabel, const LookaheadDFA<TCHAR>& ldfa, std::vector<asmjit::Label>& exitlabels)
{
    std::vector<asmjit::Label> statelabels;

    for (int i = 0; i < ldfa.get_state_num(); i++)
    {
        statelabels.push_back(as.newLabel());
    }
    for (int i = 0; i < ldfa.get_state_num(); i++)
    {
        as.bind(statelabels[i]);
        emit_state(as, rejectlabel, ldfa[i], statelabels, exitlabels);
    }
}

template<>
void LDFARoutineEM64T<char>::emit_state(asmjit::X86Assembler& as, asmjit::Label& rejectlabel, const LDFAState<char>& state, std::vector<asmjit::Label>& labels, std::vector<asmjit::Label>& exitlabels)
{
    as.movzx(CHAR_REG, asmjit::x86::byte_ptr(INPUT_REG, 0));
    as.mov(CHAR2_REG, CHAR_REG);
    as.inc(INPUT_REG);

    for (const auto& tr : state.get_transitions())
    {
        for (const auto& r : tr.label())
        {
            if (r.start() + 1 == r.end())
            {
                as.cmp(CHAR_REG, r.start());
                if (tr.dest() >= 0)
                {
                    as.je(labels[tr.dest()]);
                }
                else
                {
                    as.je(exitlabels[-tr.dest() - 1]);
                }
            }
            else
            {
                as.sub(CHAR_REG, r.start());
                as.cmp(CHAR_REG, r.end() - r.start());
                if (tr.dest() >= 0)
                {
                    as.jb(labels[tr.dest()]);
                }
                else
                {
                    as.jb(exitlabels[-tr.dest() - 1]);
                }
                as.mov(CHAR_REG, CHAR2_REG);
            }
        }
    }
    as.jmp(rejectlabel);
}

template<>
void LDFARoutineEM64T<wchar_t>::emit_state(asmjit::X86Assembler& as, asmjit::Label& rejectlabel, const LDFAState<wchar_t>& state, std::vector<asmjit::Label>& labels, std::vector<asmjit::Label>& exitlabels)
{
	as.movzx(CHAR_REG, asmjit::x86::word_ptr(INPUT_REG, 0));
	as.mov(CHAR2_REG, CHAR_REG);
	as.add(INPUT_REG, 2);

	for (const auto& tr : state.get_transitions())
	{
		for (const auto& r : tr.label())
		{
			if (r.start() + 1 == r.end())
			{
				as.cmp(CHAR_REG, r.start());
				if (tr.dest() >= 0)
				{
					as.je(labels[tr.dest()]);
				}
				else
				{
                    as.je(exitlabels[-tr.dest() - 1]);
				}
			}
			else
			{
				as.sub(CHAR_REG, r.start());
				as.cmp(CHAR_REG, r.end() - r.start());
				if (tr.dest() >= 0)
				{
					as.jb(labels[tr.dest()]);
				}
				else
				{
                    as.jb(exitlabels[-tr.dest() - 1]);
				}
				as.mov(CHAR_REG, CHAR2_REG);
			}
		}
	}
    as.jmp(rejectlabel);
}

template<typename TCHAR>
MatchRoutineEM64T<TCHAR>::MatchRoutineEM64T(const std::basic_string<TCHAR>& str, asmjit::Logger *logger)
{
    m_code.init(m_runtime.getCodeInfo());
    if (logger != NULL)
        m_code.setLogger(logger);

    asmjit::X86Assembler as(&m_code);

    emit_parser_prolog(as);

    asmjit::Label rejectlabel = as.newLabel();

    emit(as, rejectlabel, str);

    emit_parser_epilog(as);

    as.bind(rejectlabel);
    as.mov(inputreg, 0);
    as.ret();
    
    as.finalize();
}

template<>
void MatchRoutineEM64T<char>::emit(asmjit::X86Assembler& as, asmjit::Label& rejectlabel, const std::basic_string<char>& str)
{
	for (int i = 0; i < str.size(); i += 16)
	{
		int l1 = std::min(str.size() - i, (size_t)16);

		asmjit::Data128 d1;

		for (int j = 0; j < l1; j++)
		{
			d1.ub[j] = str[i + j];
		}
		
		as.vmovdqu(asmjit::x86::xmm0, asmjit::X86Mem(INPUT_REG, 0));
		//as.vpcmpistri(asmjit::x86::xmm0, , asmjit::Imm(0x18));
		as.cmp(asmjit::x86::rcx, asmjit::Imm(l1));
		as.jb(rejectlabel);
		as.add(INPUT_REG, l1);
	}
}
template<>
void MatchRoutineEM64T<wchar_t>::emit(asmjit::X86Assembler& as, asmjit::Label& rejectlabel, const std::basic_string<wchar_t>& str)
{
	for (int i = 0; i < str.size(); i += 8)
	{
		int l1 = std::min(str.size() - i, (size_t)8);

		asmjit::Data128 d1;

		for (int j = 0; j < l1; j++)
		{
			d1.uw[j] = str[i + j];
		}

		as.vmovdqu(asmjit::x86::xmm0, asmjit::X86Mem(INPUT_REG, 0));
		//as.vpcmpistri(loadreg, cc.newXmmConst(asmjit::kConstScopeGlobal, d1), asmjit::Imm(0x18), mireg);
		as.cmp(asmjit::x86::rcx, asmjit::Imm(l1));
		as.jb(rejectlabel);
		as.add(INPUT_REG, l1);
	}
}

template<typename TCHAR>
void SkipRoutineEM64T<TCHAR>::emit(asmjit::X86Assembler& as, const CharClass<TCHAR>& filter)
{
    asmjit::X86Mem filtermem = cc.newXmmConst(asmjit::kConstScopeGlobal, pack_charclass(filter));
    
    cc.vmovdqa(filterreg, filtermem);

    asmjit::Label looplabel = cc.newLabel();

    cc.bind(looplabel);

    emit_core(cc, inputreg, filterreg);

    cc.jg(looplabel);
}

template<>
void SkipRoutineEM64T<char>::emit_core(asmjit::X86Assembler& as)
{
    as.vmovdqu(asmjit::x86::xmm0, asmjit::X86Mem(INPUT_REG, 0));
    as.vpcmpistri(filterreg, asmjit::x86::xmm0, asmjit::Imm(0x14));
    as.add(INPUT_REG, asmjit::x86::rcx);
    as.cmp(asmjit::x86::rcx, 15);
}

template<>
void SkipRoutineEM64T<wchar_t>::emit_core(asmjit::X86Assembler& as)
{
    as.vmovdqu(asmjit::x86::xmm0, asmjit::X86Mem(INPUT_REG, 0));
    as.vpcmpistri(filterreg, asmjit::x86::xmm0, asmjit::Imm(0x15));
    as.sal(asmjit::x86::rcx, 1);
    as.add(INPUT_REG, asmjit::x86::rcx);
    as.cmp(asmjit::x86::rcx, 14);
}

template<typename TCHAR>
SkipRoutineEM64T<TCHAR>::SkipRoutineEM64T(const CharClass<TCHAR>& filter, asmjit::Logger *logger = NULL)
{
    m_code.init(m_runtime.getCodeInfo());
    if (logger != NULL)
        m_code.setLogger(logger);

    asmjit::X86Assembler as(&m_code);

    emit_parser_prolog(as);
    
    emit(as, filter);

    emit_parser_epilog(as);
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