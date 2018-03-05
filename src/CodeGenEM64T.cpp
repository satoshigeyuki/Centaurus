#include "DFA.hpp"
#include "ATN.hpp"
#include "asmjit/asmjit.h"
#include "CodeGenEM64T.hpp"

/*
 * Register assignment on x86/x86-64 platform
 * ATN Machine scope
 *  CONTEXT_REG     MM2/R8
 *  INPUT_REG       ESI/RSI
 *  OUTPUT_REG      EDI/RDI
 *  OUTPUT_BOUND    EDX/RDX
 *  Stack backup    MM3/R9
 * ATN Machine scope (marker writing)
 *  MARKER_REG      EAX/RAX
 * DFA Routine scope
 *  BACKUP_REG      EBX/RBX
 *  CHAR_REG        EAX/RAX
 *  CHAR2_REG       ECX/RCX
 * LDFA Routine scope
 *  PEEK_REG        EBX/RBX
 *  CHAR_REG        EAX/RAX
 *  CHAR2_REG       ECX/RCX
 * Match Routine scope
 *  LOAD_REG        XMM0
 *  INDEX_REG       ECX/RCX
 *  CHAR_REG        EAX/RAX
 * Skip Routine scope
 *  LOAD_REG        XMM0
 *  PATTERN_REG     XMM1
 *  INDEX_REG       ECX/RCX
 */

//Parser/Machine scope registers
#define CONTEXT_REG asmjit::x86::r8
#define INPUT_REG asmjit::x86::rsi
#define OUTPUT_REG asmjit::x86::rdi
#define OUTPUT_BOUND_REG asmjit::x86::rdx
#define INPUT_BASE_REG asmjit::x86::rbp
#define MARKER_REG asmjit::x86::rax
#define ID_REG asmjit::x86::rbx

//DFA/LDFA routine scope registers
#define BACKUP_REG asmjit::x86::rbx
#define PEEK_REG asmjit::x86::rbx
#define CHAR_REG asmjit::x86::rax
#define CHAR2_REG asmjit::x86::rcx

//Match/Skip routine scope registers
#define LOAD_REG asmjit::x86::xmm0
#define PATTERN_REG asmjit::x86::xmm1
#define INDEX_REG asmjit::x86::rcx

#if defined(CENTAURUS_BUILD_WINDOWS)
#define ARG1_REG asmjit::x86::rcx
#define ARG1_STACK_OFFSET (16)
#define ARG2_STACK_OFFSET (24)
#define ARG3_STACK_OFFSET (56)
#elif defined(CENTAURUS_BUILD_LINUX)
#define ARG1_REG asmjit::x86::rdi
#define ARG1_STACK_OFFSET (40)
#define ARG2_STACK_OFFSET (32)
#define ARG3_STACK_OFFSET (24)
#endif

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

    //Save RSP for bailout
    as.mov(asmjit::x86::r9, asmjit::x86::rsp);
}

static void emit_parser_epilog(asmjit::X86Assembler& as, asmjit::Label& rejectlabel)
{
    asmjit::Label acceptlabel = as.newLabel();

    as.mov(asmjit::x86::rax, INPUT_REG);
    as.jmp(acceptlabel);

    //Jump to this label is a long jump. Discard everything on the stack.
    as.bind(rejectlabel);
    as.mov(asmjit::x86::rax, 0);
    as.mov(asmjit::x86::rsp, asmjit::x86::r9);

    as.bind(acceptlabel);

    as.pop(asmjit::x86::rbx);
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
ParserEM64T<TCHAR>::ParserEM64T(const Grammar<TCHAR>& grammar, asmjit::Logger *logger, asmjit::ErrorHandler *errhandler)
{
    m_code.init(m_runtime.getCodeInfo());
    if (logger != NULL)
        m_code.setLogger(logger);
    if (errhandler != NULL)
        m_code.setErrorHandler(errhandler);

    asmjit::X86Assembler as(&m_code);

    std::unordered_map<Identifier, asmjit::Label> machine_map;

    emit_parser_prolog(as);

    as.mov(CONTEXT_REG, asmjit::X86Mem(asmjit::x86::rsp, ARG1_STACK_OFFSET));
    as.mov(INPUT_REG, asmjit::X86Mem(asmjit::x86::rsp, ARG2_STACK_OFFSET));
    as.mov(OUTPUT_REG, asmjit::X86Mem(asmjit::x86::rsp, ARG3_STACK_OFFSET));
    as.mov(OUTPUT_BOUND_REG, OUTPUT_REG);
    as.add(OUTPUT_BOUND_REG, asmjit::Imm(AST_BUF_SIZE));
    as.mov(INPUT_BASE_REG, INPUT_REG);

    MyConstPool pool(as);

    pool.load_charclass_filter(PATTERN_REG, m_skipfilter);

    asmjit::Label rejectlabel = as.newLabel();

    {
        for (const auto& p : grammar.get_machines())
        {
            machine_map.emplace(p.first, as.newLabel());
        }

        as.call(machine_map[grammar.get_root_id()]);
    }

    asmjit::Label finishlabel = as.newLabel();
    as.jmp(finishlabel);

    CompositeATN<TCHAR> catn(grammar);

    for (const auto& p : grammar.get_machines())
    {
        as.bind(machine_map[p.first]);

        emit_machine(as, p.second, machine_map, catn, p.first, rejectlabel, pool);
    }

    as.bind(finishlabel);

    emit_parser_epilog(as, rejectlabel);

    pool.embed();

    as.finalize();

    m_runtime.add(&m_func, &m_code);
}

template<typename TCHAR>
DryParserEM64T<TCHAR>::DryParserEM64T(const Grammar<TCHAR>& grammar, asmjit::Logger *logger)
{
    m_code.init(m_runtime.getCodeInfo());
    if (logger != NULL)
        m_code.setLogger(logger);

    asmjit::X86Assembler as(&m_code);

    std::unordered_map<Identifier, asmjit::Label> machine_map;

    emit_parser_prolog(as);

    as.mov(INPUT_REG, asmjit::X86Mem(asmjit::x86::rsp, ARG1_STACK_OFFSET));

    MyConstPool pool(as);

    pool.load_charclass_filter(PATTERN_REG, m_skipfilter);

    asmjit::Label rejectlabel = as.newLabel();

    {
        for (const auto& p : grammar.get_machines())
        {
            machine_map.emplace(p.first, as.newLabel());
        }

        as.call(machine_map[grammar.get_root_id()]);
    }

    emit_parser_epilog(as, rejectlabel);

    CompositeATN<TCHAR> catn(grammar);

    for (const auto& p : grammar.get_machines())
    {
        as.bind(machine_map[p.first]);

        emit_machine(as, p.second, machine_map, catn, p.first, rejectlabel, pool);
    }

    pool.embed();

    as.finalize();
}

template<> CharClass<char> ParserEM64T<char>::m_skipfilter({ ' ', '\t', '\r', '\n' });
template<> CharClass<wchar_t> ParserEM64T<wchar_t>::m_skipfilter({ L' ', L'\t', L'\r', L'\n' });

template<> CharClass<char> DryParserEM64T<char>::m_skipfilter({' ', '\t', '\r', '\n'});
template<> CharClass<wchar_t> DryParserEM64T<wchar_t>::m_skipfilter({L' ', L'\t', L'\r', L'\n'});

template<typename TCHAR>
void ParserEM64T<TCHAR>::emit_machine(asmjit::X86Assembler& as, const ATNMachine<TCHAR>& machine, std::unordered_map<Identifier, asmjit::Label>& machine_map, const CompositeATN<TCHAR>& catn, const Identifier& id, asmjit::Label& rejectlabel, MyConstPool& pool)
{
    std::vector<asmjit::Label> statelabels;

    for (int i = 0; i < machine.get_node_num(); i++)
    {
        statelabels.push_back(as.newLabel());
    }

    asmjit::Label requestpage1_label = as.newLabel();
    asmjit::Label requestpage2_label = as.newLabel();

    {
        //Write the start marker to the AST buffer.
        //Structure of the start marker (64 bits, Little Endian):
        //64  63       48               0
        //+---+--------+----------------+
        //| 1 | ATN ID | Start Position |
        //+---+--------+----------------+
        
        as.mov(MARKER_REG, INPUT_REG);
        as.sub(MARKER_REG, INPUT_BASE_REG);
        as.mov(ID_REG, machine.get_unique_id() | (1 << 15));
        as.shl(ID_REG, 48);
        as.or_(MARKER_REG, ID_REG);
        as.movnti(asmjit::X86Mem(OUTPUT_REG, 0), MARKER_REG);
        as.add(OUTPUT_REG, 8);
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
            MatchRoutineEM64T<TCHAR>::emit(as, pool, rejectlabel, node.get_literal());
            break;
        case ATNNodeType::Nonterminal:
            as.call(machine_map[node.get_invoke()]);
            break;
        case ATNNodeType::RegularTerminal:
            DFARoutineEM64T<TCHAR>::emit(as, rejectlabel, DFA<TCHAR>(node.get_nfa()));
            break;
        case ATNNodeType::WhiteSpace:
            SkipRoutineEM64T<TCHAR>::emit(as);
            break;
        }

        int outbound_num = node.get_transitions().size();
        if (outbound_num == 0)
        {
            //Write the end marker to the AST buffer.
            //Structure of the end marker:
            //64  63       48             0
            //+---+--------+--------------+
            //| 0 | ATN ID | End Position |
            //+---+--------+--------------+

            as.mov(MARKER_REG, INPUT_REG);
            as.sub(MARKER_REG, INPUT_BASE_REG);
            as.mov(ID_REG, machine.get_unique_id());
            as.shl(ID_REG, 48);
            as.or_(MARKER_REG, ID_REG);
            as.movnti(asmjit::X86Mem(OUTPUT_REG, 0), MARKER_REG);
            as.add(OUTPUT_REG, 8);
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

    as.bind(requestpage1_label);
    {
        as.push(INPUT_REG);
        as.push(CONTEXT_REG);
        as.mov(ARG1_REG, asmjit::X86Mem(asmjit::x86::rsp, 0));
        as.sub(asmjit::x86::rsp, 32);
        as.call((uint64_t)request_page);
        as.add(asmjit::x86::rsp, 32);
        as.mov(OUTPUT_REG, asmjit::x86::rax);
        as.mov(OUTPUT_BOUND_REG, OUTPUT_REG);
        as.add(OUTPUT_BOUND_REG, AST_BUF_SIZE);
        as.pop(CONTEXT_REG);
        as.pop(INPUT_REG);

        as.jmp(statelabels[0]);
    }
    as.bind(requestpage2_label);
    {
        as.push(INPUT_REG);
        as.push(CONTEXT_REG);
        as.mov(ARG1_REG, asmjit::X86Mem(asmjit::x86::rsp, 0));
#if defined(CENTAURUS_BUILD_WINDOWS)
        as.sub(asmjit::x86::rsp, 32);
#endif
        as.call((uint64_t)request_page);
#if defined(CENTAURUS_BUILD_WINDOWS)
        as.add(asmjit::x86::rsp, 32);
#endif
        as.mov(OUTPUT_REG, asmjit::x86::rax);
        as.mov(OUTPUT_BOUND_REG, OUTPUT_REG);
        as.add(OUTPUT_BOUND_REG, AST_BUF_SIZE);
        as.pop(CONTEXT_REG);
        as.pop(INPUT_REG);

        as.ret();
    }
}

template<typename TCHAR>
void * ParserEM64T<TCHAR>::request_page(void *context)
{
    ParserEM64T<TCHAR> *parser = (ParserEM64T<TCHAR> *)context;

    parser->m_flipcount++;

    return parser->m_buffer;
}

template<typename TCHAR>
void DryParserEM64T<TCHAR>::emit_machine(asmjit::X86Assembler& as, const ATNMachine<TCHAR>& machine, std::unordered_map<Identifier, asmjit::Label>& machine_map, const CompositeATN<TCHAR>& catn, const Identifier& id, asmjit::Label& rejectlabel, MyConstPool& pool)
{
    std::vector<asmjit::Label> statelabels;

    for (int i = 0; i < machine.get_node_num(); i++)
    {
        statelabels.push_back(as.newLabel());
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
            MatchRoutineEM64T<TCHAR>::emit(as, pool, rejectlabel, node.get_literal());
            break;
        case ATNNodeType::Nonterminal:
            as.call(machine_map[node.get_invoke()]);
            break;
        case ATNNodeType::RegularTerminal:
            DFARoutineEM64T<TCHAR>::emit(as, rejectlabel, DFA<TCHAR>(node.get_nfa()));
            break;
        case ATNNodeType::WhiteSpace:
            SkipRoutineEM64T<TCHAR>::emit(as);
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
}

template<typename TCHAR>
DFARoutineEM64T<TCHAR>::DFARoutineEM64T(const DFA<TCHAR>& dfa, asmjit::Logger *logger)
{
    m_code.init(m_runtime.getCodeInfo());
    if (logger != NULL)
        m_code.setLogger(logger);

    asmjit::X86Assembler as(&m_code);

    emit_parser_prolog(as);

    as.mov(INPUT_REG, asmjit::X86Mem(asmjit::x86::rsp, ARG1_STACK_OFFSET));

    asmjit::Label rejectlabel = as.newLabel();

    emit(as, rejectlabel, dfa);

    emit_parser_epilog(as, rejectlabel);

    as.finalize();
}

template<typename TCHAR>
void DFARoutineEM64T<TCHAR>::emit(asmjit::X86Assembler& as, asmjit::Label& rejectlabel, const DFA<TCHAR>& dfa)
{
    std::vector<asmjit::Label> statelabels;

    for (int i = 0; i < dfa.get_state_num(); i++)
    {
        statelabels.push_back(as.newLabel());
    }

    asmjit::Label finishlabel = as.newLabel();

    as.mov(BACKUP_REG, 0);
    as.jmp(statelabels[0]);

    for (int i = 0; i < dfa.get_state_num(); i++)
    {
        as.bind(statelabels[i]);

        emit_state(as, finishlabel, dfa[i], statelabels);
    }
    as.bind(finishlabel);
    as.mov(INPUT_REG, BACKUP_REG);
    as.cmp(INPUT_REG, 0);
    as.jz(rejectlabel);
}

template<typename TCHAR>
void DFARoutineEM64T<TCHAR>::emit_state(asmjit::X86Assembler& as, asmjit::Label& rejectlabel, const DFAState<TCHAR>& state, std::vector<asmjit::Label>& labels)
{
    //Read the character from stream and advance the input position
    if (sizeof(TCHAR) == 1)
        as.movzx(CHAR_REG, asmjit::x86::byte_ptr(INPUT_REG, 0));
    else
        as.movzx(CHAR_REG, asmjit::x86::word_ptr(INPUT_REG, 0));
    as.mov(CHAR2_REG, CHAR_REG);
	if (state.is_accept_state())
		as.mov(BACKUP_REG, INPUT_REG);
    if (sizeof(TCHAR) == 1)
        as.inc(INPUT_REG);
    else
        as.add(INPUT_REG, 2);

    if (state.get_transitions().size() == 1 && state.get_transitions()[0].label().size() == 1)
    {
        const NFATransition<TCHAR>& tr = state.get_transitions()[0];
        const Range<TCHAR>& r = tr.label()[0];
        {
            if (r.start() + 1 == r.end())
            {
                as.cmp(CHAR_REG, r.start());
                as.jne(rejectlabel);
            }
            else
            {
                as.sub(CHAR_REG, r.start());
                as.cmp(CHAR_REG, r.end() - r.start());
                as.jnb(rejectlabel);
            }
        }
        as.jmp(labels[tr.dest()]);
    }
    else
    {
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
}

template<typename TCHAR>
LDFARoutineEM64T<TCHAR>::LDFARoutineEM64T(const LookaheadDFA<TCHAR>& ldfa, asmjit::Logger *logger)
{
    m_code.init(m_runtime.getCodeInfo());
    if (logger != NULL)
        m_code.setLogger(logger);

    asmjit::X86Assembler as(&m_code);

    emit_parser_prolog(as);

    as.mov(INPUT_REG, asmjit::X86Mem(asmjit::x86::rsp, ARG1_STACK_OFFSET));

    asmjit::Label rejectlabel = as.newLabel();

    std::vector<asmjit::Label> exitlabels;

    int decision_num = ldfa.get_color_num();
    for (int i = 0; i < decision_num; i++)
    {
        exitlabels.push_back(as.newLabel());
    }

    emit(as, rejectlabel, ldfa, exitlabels);
    as.jmp(rejectlabel);

    asmjit::Label finishlabel = as.newLabel();
    for (int i = 0; i < decision_num; i++)
    {
        as.bind(exitlabels[i]);
        as.mov(INPUT_REG, asmjit::Imm(i + 1));
        as.jmp(finishlabel);
    }

    as.bind(finishlabel);
    emit_parser_epilog(as, rejectlabel);
   
    as.finalize();
}

template<typename TCHAR>
void LDFARoutineEM64T<TCHAR>::emit(asmjit::X86Assembler& as, asmjit::Label& rejectlabel, const LookaheadDFA<TCHAR>& ldfa, std::vector<asmjit::Label>& exitlabels)
{
    std::vector<asmjit::Label> statelabels;

    as.mov(PEEK_REG, INPUT_REG);

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

template<typename TCHAR>
void LDFARoutineEM64T<TCHAR>::emit_state(asmjit::X86Assembler& as, asmjit::Label& rejectlabel, const LDFAState<TCHAR>& state, std::vector<asmjit::Label>& labels, std::vector<asmjit::Label>& exitlabels)
{
    if (sizeof(TCHAR) == 1)
        as.movzx(CHAR_REG, asmjit::x86::byte_ptr(PEEK_REG, 0));
    else
        as.movzx(CHAR_REG, asmjit::x86::word_ptr(PEEK_REG, 0));
    as.mov(CHAR2_REG, CHAR_REG);
    if (sizeof(TCHAR) == 1)
        as.inc(PEEK_REG);
    else
        as.add(PEEK_REG, 2);

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

    as.mov(INPUT_REG, asmjit::X86Mem(asmjit::x86::rsp, ARG1_STACK_OFFSET));

    MyConstPool pool(as);

    asmjit::Label rejectlabel = as.newLabel();

    emit(as, pool, rejectlabel, str);

    emit_parser_epilog(as, rejectlabel);

    pool.embed();
    
    as.finalize();
}

template<typename TCHAR>
void MatchRoutineEM64T<TCHAR>::emit(asmjit::X86Assembler& as, MyConstPool& pool, asmjit::Label& rejectlabel, const std::basic_string<TCHAR>& str)
{
    if (str.size() < 8)
    {
        for (int i = 0; i < str.size(); i++)
        {
            if (sizeof(TCHAR) == 1)
            {
                as.movzx(CHAR_REG, asmjit::x86::byte_ptr(INPUT_REG, 0));
                as.inc(INPUT_REG);
            }
            else
            {
                as.movzx(CHAR_REG, asmjit::x86::word_ptr(INPUT_REG, 0));
                as.add(INPUT_REG, 2);
            }
            as.cmp(CHAR_REG, str[i]);
            as.jne(rejectlabel);
        }
    }
    else
    {
        for (int i = 0; i < str.size(); i += 16 / sizeof(TCHAR))
        {
            int l1 = std::min(str.size() - i, (size_t)(16 / sizeof(TCHAR)));

            asmjit::Data128 d1;

            for (int j = 0; j < l1; j++)
            {
                if (sizeof(TCHAR) == 1)
                    d1.ub[j] = str[i + j];
                else
                    d1.uw[j] = str[i + j];
            }

            as.vmovdqu(LOAD_REG, asmjit::X86Mem(INPUT_REG, 0));
            as.vpcmpistri(LOAD_REG, pool.add(d1), asmjit::Imm(0x18));
            as.cmp(INDEX_REG, asmjit::Imm(l1));
            as.jb(rejectlabel);
            as.add(INPUT_REG, l1);
        }
    }
}

template<typename TCHAR>
void SkipRoutineEM64T<TCHAR>::emit(asmjit::X86Assembler& as)
{
    asmjit::Label looplabel = as.newLabel();

    as.bind(looplabel);

    as.vmovdqu(LOAD_REG, asmjit::X86Mem(INPUT_REG, 0));
    as.vpcmpistri(PATTERN_REG, LOAD_REG, asmjit::Imm(sizeof(TCHAR) == 1 ? 0x14 : 0x15));
    if (sizeof(TCHAR) == 2)
        as.sal(INDEX_REG, 1);
    as.add(INPUT_REG, INDEX_REG);
    as.cmp(INDEX_REG, 15);

    as.jg(looplabel);
}

template<typename TCHAR>
SkipRoutineEM64T<TCHAR>::SkipRoutineEM64T(const CharClass<TCHAR>& filter, asmjit::Logger *logger)
{
    m_code.init(m_runtime.getCodeInfo());
    if (logger != NULL)
        m_code.setLogger(logger);

    asmjit::X86Assembler as(&m_code);

    emit_parser_prolog(as);

    as.mov(INPUT_REG, asmjit::X86Mem(asmjit::x86::rsp, ARG1_STACK_OFFSET));

    MyConstPool pool(as);

    asmjit::Label rejectlabel = as.newLabel();

    pool.load_charclass_filter(PATTERN_REG, filter);
    
    emit(as);

    emit_parser_epilog(as, rejectlabel);

    pool.embed();

    as.finalize();
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
