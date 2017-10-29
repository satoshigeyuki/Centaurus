#pragma once

#include "nfa.hpp"
#include "identifier.hpp"
#include "stream.hpp"
#include "charclass.hpp"

namespace Centaurus
{
enum class ATNTransitionType
{
    Epsilon,
    SingleCharLookup,
    LiteralLookup
};

template<typename TCHAR> class ATNTransition
{
    template<typename T> friend std::ostream& operator<<(std::ostream& os, const ATNTransition<T>& tr);

    ATNTransitionType m_type;
    CharClass<TCHAR> m_class;
    int m_dest;
public:
    ATNTransition(const CharClass<TCHAR>& cls, int dest)
        : m_type(ATNTransitionType::SingleCharLookup), m_class(cls), m_dest(dest)
    {
    }
    ATNTransition(int dest)
        : m_type(ATNTransitionType::Epsilon), m_dest(dest)
    {
    }
    virtual ~ATNTransition()
    {
    }
    int dest() const
    {
        return m_dest;
    }
    void add_class(const CharClass<TCHAR>& cc)
    {
        m_class |= cc;
    }
    bool is_epsilon() const
    {
        return m_type == ATNTransitionType::Epsilon;
    }
    ATNTransitionType type() const
    {
        return m_type;
    }
    const CharClass<TCHAR>& get_class() const
    {
        return m_class;
    }
};
template<typename TCHAR>
std::ostream& operator<<(std::ostream& os, const ATNTransition<TCHAR>& tr)
{
    switch (tr.m_type)
    {
    case ATNTransitionType::Epsilon:
        os << 'E';
        break;
    case ATNTransitionType::SingleCharLookup:
        os << tr.m_class;
        break;
    case ATNTransitionType::LiteralLookup:
        break;
    }
    return os;
}

enum class ATNNodeType
{
    Epsilon,
    Nonterminal,
    LiteralTerminal,
    RegularTerminal
};

template<typename TCHAR> class ATNNode
{
    std::vector<ATNTransition<TCHAR> > m_transitions;
    ATNNodeType m_type;
    Identifier m_invoke;
    NFA<TCHAR> m_nfa;
    std::wstring m_literal;
private:
    void parse_literal(Stream& stream)
    {
        Stream::Sentry sentry = stream.sentry();

        wchar_t leader = stream.get();

        wchar_t ch = stream.get();

        for (; ch != L'\0' && ch != L'\'' && ch != L'"'; ch = stream.get())
            ;

        if (leader != ch)
            throw stream.unexpected(ch);

        m_literal = stream.cut(sentry);
    }
    void parse(Stream& stream)
    {
        wchar_t ch = stream.peek();

        if (Identifier::is_symbol_leader(ch))
        {
            m_invoke.parse(stream);
            m_type = ATNNodeType::Nonterminal;
        }
        else if (ch == L'/')
        {
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
    }
public:
    ATNNode()
        : m_type(ATNNodeType::Epsilon)
    {
    }
    ATNNode(Stream& stream)
    {
        parse(stream);
    }
    virtual ~ATNNode()
    {
    }
    void add_transition(const ATNTransition<TCHAR>& transition)
    {
        m_transitions.push_back(transition);
    }
    void add_transition(const CharClass<TCHAR>& cc, int dest)
    {
        if (!cc.is_epsilon())
        {
            for (auto& tr : m_transitions)
            {
                if (tr.type() == ATNTransitionType::SingleCharLookup && tr.dest() == dest)
                {
                    tr.add_class(cc);
                    return;
                }
            }
            m_transitions.emplace_back(cc, dest);
        }
        else
        {
            m_transitions.emplace_back(dest);
        }
    }
    void print(std::ostream& os, int from) const
    {
        for (const auto& tr : m_transitions)
        {
            os << "S" << from << " -> " << "S" << tr.dest() << " [ label=\"";
            os << tr;
            os << "\" ];" << std::endl;
        }
    }
};

template<typename TCHAR> class ATN
{
    std::vector<ATNNode<TCHAR> > m_nodes;
public:
    void parse(Stream& stream)
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
    int add_node(int from)
    {
        m_nodes[from].add_transition(m_nodes.size());
        m_nodes.emplace_back();
        return m_nodes.size() - 1;
    }
    void add_transition_to(int dest)
    {
        m_nodes.back().add_transition(dest);
    }
    void add_transition_from(int src)
    {
        m_nodes[src].add_transition(m_nodes.size() - 1);
    }
    ATN(Stream& stream)
    {
        m_nodes.emplace_back();

        parse(stream);
    }
    ATN()
    {
        m_nodes.emplace_back();
    }
    virtual ~ATN()
    {
    }
    virtual void print_node(std::ostream& os, int index)
    {
        os << index;
    }
    void print(std::ostream& os, const std::string& graph_name)
    {
        os << "digraph " << graph_name << " {" << std::endl;

        os << "graph [ charset=\"UTF-8\", style=\"filled\" ];" << std::endl;

        os << "node [ style=\"solid,filled\" ];" << std::endl;

        os << "edge [ style=\"solid\" ];" << std::endl;

        for (unsigned int i = 0; i < m_nodes.size(); i++)
        {
            os << "S" << i << " [ label=\"";
            print_node(os, i);
            os << "\", shape=circle ];" << std::endl;
        }

        for (unsigned int i = 0; i < m_nodes.size(); i++)
        {
            m_nodes[i].print(os, i);
        }

        os << "}" << std::endl;
    }
};
}
