#include "ATN.hpp"

namespace Centaurus
{
template<typename TCHAR>
void ATNNode<TCHAR>::parse_literal(Stream& stream)
{
    wchar_t leader = stream.get();

    wchar_t ch = stream.get();

    for (; ch != L'\0' && ch != L'\'' && ch != L'"'; ch = stream.get())
    {
        m_literal.push_back(wide_to_target<TCHAR>(ch));
    }

    if (leader != ch)
        throw stream.unexpected(ch);
}

template<typename TCHAR>
void ATNNode<TCHAR>::parse(Stream& stream)
{
    wchar_t ch = stream.peek();

    if (Identifier::is_symbol_leader(ch))
    {
        m_invoke.parse(stream);
        m_type = ATNNodeType::Nonterminal;
    }
    else if (ch == L'/')
    {
        stream.discard();
        m_nfa.parse(stream);
        m_type = ATNNodeType::RegularTerminal;
    }
    else if (ch == L'\'' || ch == L'"')
    {
        parse_literal(stream);
        m_type = ATNNodeType::LiteralTerminal;
    }
    else
    {
        throw stream.unexpected(ch);
    }

    ch = stream.skip_whitespace();

    if (ch == L'{')
    {
		stream.discard();

        ch = stream.skip_whitespace();

        if (ch == L'}')
            throw stream.unexpected(ch);

        int id = 0;
        for (; L'0' <= ch && ch <= L'9'; ch = stream.peek())
        {
            id = id * 10 + static_cast<int>(ch - L'0');
            stream.discard();
        }
        
        ch = stream.skip_whitespace();
        if (ch != L'}')
            throw stream.unexpected(ch);
        stream.discard();

		stream.skip_whitespace();

        m_localid = id;
    }
}

template<typename TCHAR>
void ATNMachine<TCHAR>::parse(Stream& stream)
{
    std::vector<int> terminal_states;

    wchar_t ch = stream.skip_whitespace();

    for (; ch != L';'; ch = stream.skip_whitespace())
    {
        if (ch == L'\0')
            throw stream.unexpected(ch);

        if (ch == L':' || ch == L'|')
        {
            stream.discard();
            terminal_states.push_back(add_node(0));
        }
        else
        {
            if (terminal_states.empty())
                throw stream.unexpected(ch);
            m_nodes.back().add_transition(m_nodes.size());
            m_nodes.emplace_back(ATNNodeType::WhiteSpace);
            m_nodes.back().add_transition(m_nodes.size());
            m_nodes.emplace_back(stream);
            terminal_states.back() = m_nodes.size() - 1;
        }
    }
    stream.discard();

    m_nodes.emplace_back();
    int final_node = m_nodes.size() - 1;
    for (int from : terminal_states)
    {
        m_nodes[from].add_transition(final_node);
    }
}

template class ATNMachine<char>;
template class ATNMachine<wchar_t>;
}
