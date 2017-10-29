#pragma once

#include <stack>
#include <unordered_map>

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
    std::basic_string<TCHAR> m_literal;
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
    ATNTransition(const std::basic_string<TCHAR>& literal, int dest)
        : m_type(ATNTransition::LiteralLookup), m_literal(literal), m_dest(dest)
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
        break;
    case ATNTransitionType::SingleCharLookup:
        os << tr.m_class;
        break;
    case ATNTransitionType::LiteralLookup:
        os << "\"" << tr.m_literal << "\"";
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

template<typename TCHAR> class ATNPrinter;

template<typename TCHAR> class ATNNode
{
    friend class ATNPrinter<TCHAR>;

    std::vector<ATNTransition<TCHAR> > m_transitions;
    ATNNodeType m_type;
    Identifier m_invoke;
    NFA<TCHAR> m_nfa;
    std::basic_string<TCHAR> m_literal;
private:
    void parse_literal(Stream& stream)
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
    std::pair<std::string, std::string> get_entry_exit(const std::string& prefix) const
    {
        if (m_type == ATNNodeType::RegularTerminal)
        {
            return std::pair<std::string, std::string>(m_nfa.get_entry(prefix), m_nfa.get_exit(prefix));
        }
        else
        {
            return std::pair<std::string, std::string>(prefix, prefix);
        }
    }
    void print(std::ostream& os, const std::string& prefix) const
    {
        switch (m_type)
        {
        case ATNNodeType::Epsilon:
            os << prefix << " [ label=\"\" ];" << std::endl;
            break;
        case ATNNodeType::Nonterminal:
            os << prefix << " [ label=\"" << m_invoke.narrow() << "\" ];" << std::endl;
            break;
        case ATNNodeType::LiteralTerminal:
            os << prefix << " [ label=\"" << m_literal << "\" ];" << std::endl;
            break;
        case ATNNodeType::RegularTerminal:
            m_nfa.print_subgraph(os, prefix);
            break;
        }
    }
    ATNNodeType type() const
    {
        return m_type;
    }
    bool is_nonterminal() const
    {
        return m_type == ATNNodeType::Nonterminal;
    }
};

template<typename TCHAR> class ATN
{
    friend class ATNPrinter<TCHAR>;

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
    const ATNNode<TCHAR>& get_node(int index) const
    {
        return m_nodes[index];
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
    ATN(ATN&& atn)
        : m_nodes(std::move(atn.m_nodes))
    {
    }
    virtual ~ATN()
    {
    }
};

template<typename TCHAR>
class ATNPrinter
{
    const std::unordered_map<Identifier, ATN<TCHAR> >& m_networks;
    std::vector<Identifier> m_stack;
    int m_counter, m_maxdepth;
private:
    std::pair<std::string, std::string> print_atn(std::ostream& os, const Identifier& key)
    {
        std::string prefix = key.narrow() + std::to_string(m_counter++);

        os << "subgraph cluster_" << prefix << " {" << std::endl;

        const ATN<TCHAR>& atn = m_networks.at(key);

        std::vector<std::pair<std::string, std::string> > entry_exit_nodes(atn.m_nodes.size());

        for (unsigned int i = 0; i < atn.m_nodes.size(); i++)
        {
            const ATNNode<TCHAR>& node = atn.m_nodes[i];
            std::string node_name = prefix + "_N" + std::to_string(i);

            if (node.is_nonterminal())
            {
                if (m_stack.size() < (size_t)m_maxdepth && std::find(m_stack.begin(), m_stack.end(), node.m_invoke) == m_stack.end())
                {
                    m_stack.push_back(node.m_invoke);
                    entry_exit_nodes[i] = print_atn(os, node.m_invoke);
                    m_stack.pop_back();
                }
                else
                {
                    entry_exit_nodes[i] = node.get_entry_exit(node_name);
                    node.print(os, node_name);
                }
            }
            else
            {
                entry_exit_nodes[i] = node.get_entry_exit(node_name);
                node.print(os, node_name);
            }
        }

        for (unsigned int i = 0; i < atn.m_nodes.size(); i++)
        {
            const ATNNode<TCHAR>& node = atn.m_nodes[i];
            const std::string& exit = entry_exit_nodes[i].second;
            for (const auto& t : node.m_transitions)
            {
                const std::string& entry = entry_exit_nodes[t.dest()].first;

                os << exit << " -> " << entry << " [ label=\"";
                os << t;
                os << "\" ];" << std::endl;
            }
        }

        os << "label = \"" << key.narrow() << "\";" << std::endl;
        os << "}" << std::endl;

        std::string entry_node = prefix + "_N0";
        std::string exit_node = prefix + "_N" + std::to_string(atn.m_nodes.size() - 1);

        return std::pair<std::string, std::string>(entry_node, exit_node);
    }
public:
    ATNPrinter(const std::unordered_map<Identifier, ATN<TCHAR> >& networks, int maxdepth)
        : m_networks(networks), m_counter(0), m_maxdepth(maxdepth)
    {
    }
    virtual ~ATNPrinter()
    {
    }
    void print(std::ostream& os, const Identifier& key)
    {
        print_atn(os, key);
    }
};
}
