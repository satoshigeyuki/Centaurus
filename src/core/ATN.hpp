#pragma once

#include <stack>
#include <unordered_map>

#include "NFA.hpp"
#include "Identifier.hpp"
#include "Stream.hpp"
#include "CharClass.hpp"

namespace Centaurus
{
enum class ATNTransitionType
{
    Epsilon,
	Action
};

template<typename TCHAR> class ATNTransition
{
    ATNTransitionType m_type;
    int m_dest, m_tag;
public:
    ATNTransition(int dest)
        : m_type(ATNTransitionType::Epsilon), m_dest(dest)
    {
    }
    ATNTransition(int dest, int tag = 0)
        : m_type(ATNTransitionType::Epsilon), m_dest(dest), m_tag(tag)
    {
    }
	ATNTransition(ATNTransitionType type, int dest, int tag = 0)
		: m_type(type), m_dest(dest), m_tag(tag)
	{
	}
	ATNTransition<TCHAR> offset(int off) const
	{
		return ATNTransition<TCHAR>(m_type, m_dest + off, m_tag);
	}
    virtual ~ATNTransition()
    {
    }
	ATNTransitionType type() const
	{
		return m_type;
	}
    int dest() const
    {
        return m_dest;
    }
    void dest(int dest)
    {
        m_dest = dest;
    }
	int tag() const
	{
		return m_tag;
	}
};

template<typename TCHAR> std::ostream& operator<<(std::ostream& os, const ATNTransition<TCHAR>& tr)
{
	return os;
}
template<typename TCHAR> std::wostream& operator<<(std::wostream& os, const ATNTransition<TCHAR>& tr)
{
	return os;
}

enum class ATNNodeType
{
    Blank,
    Nonterminal,
    LiteralTerminal,
    RegularTerminal,
    WhiteSpace
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
    int m_localid;
private:
    void parse_literal(Stream& stream);
    void parse(Stream& stream);
public:
    ATNNode()
        : m_type(ATNNodeType::Blank), m_localid(-1)
    {
    }
    ATNNode(ATNNodeType type)
        : m_type(type), m_localid(-1)
    {
    }
    ATNNode(Stream& stream)
    {
        parse(stream);
    }
    ATNNode(ATNNode<TCHAR>&& old)
        : m_transitions(std::move(old.m_transitions)), m_type(old.m_type), m_invoke(std::move(old.m_invoke)), m_nfa(std::move(old.m_nfa)), m_literal(std::move(old.m_literal)), m_localid(old.m_localid)
    {
    }
    ATNNode(const ATNNode<TCHAR>& old)
        : m_transitions(old.m_transitions), m_type(old.m_type), m_invoke(old.m_invoke), m_nfa(old.m_nfa), m_literal(old.m_literal), m_localid(old.m_localid)
    {
    }
    ATNNode(const ATNNode<TCHAR>& old, std::vector<ATNTransition<TCHAR> >&& transitions)
        : m_transitions(transitions), m_type(old.m_type), m_invoke(old.m_invoke), m_nfa(old.m_nfa), m_literal(old.m_literal), m_localid(old.m_localid)
    {
    }
	ATNNode<TCHAR> offset(int off) const
	{
		std::vector<ATNTransition<TCHAR> > new_transitions;

		for (const auto& t : m_transitions)
		{
			new_transitions.push_back(t.offset());
		}

		return ATNNode<TCHAR>(*this, new_transitions);
	}
    virtual ~ATNNode()
    {
    }
    void add_transition(const ATNTransition<TCHAR>& transition)
    {
        m_transitions.push_back(transition);
    }
    void add_transition(int dest, int priority = 0)
    {
        m_transitions.emplace_back(dest, priority);
    }
    /*void add_transition(const CharClass<TCHAR>& cc, int dest)
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
    }*/
	std::pair<std::wstring, std::wstring> get_entry_exit_wide(const std::wstring& prefix) const
	{
		if (m_type == ATNNodeType::RegularTerminal)
		{
			return std::pair<std::wstring, std::wstring>(m_nfa.get_entry_wide(prefix), m_nfa.get_exit_wide(prefix));
		}
		else
		{
			return std::pair<std::wstring, std::wstring>(prefix, prefix);
		}
	}
	void print(std::wostream& os, int index, const std::wstring& prefix) const
	{
		switch (m_type)
		{
		case ATNNodeType::Blank:
			os << prefix << L" [ label=\"\", xlabel=\"[" << index << L"]\" ];" << std::endl;
			break;
		case ATNNodeType::Nonterminal:
			os << prefix << L" [ label=\"" << m_invoke << L"\", xlabel=\"[" << index << "]\" ];" << std::endl;
			break;
		case ATNNodeType::LiteralTerminal:
			os << prefix << L" [ label=\"\\\"\\\"" << /*m_literal <<*/ L"\", xlabel=\"[" << index << L"]\" ];" << std::endl;
			break;
		case ATNNodeType::RegularTerminal:
			m_nfa.print_subgraph(os, index, prefix);
			break;
		case ATNNodeType::WhiteSpace:
			os << prefix << L" [ label=\"[ ]\", xlabel=\"[" << index << L"]\" ];" << std::endl;
			break;
		}
	}
    ATNNodeType type() const
    {
        return m_type;
    }
    const std::vector<ATNTransition<TCHAR> >& get_transitions() const
    {
        return m_transitions;
    }
    std::vector<ATNTransition<TCHAR> >& get_transitions()
    {
        return m_transitions;
    }
    const ATNTransition<TCHAR>& get_transition(int index) const
    {
        return m_transitions[index];
    }
    bool is_nonterminal() const
    {
        return m_type == ATNNodeType::Nonterminal;
    }
    bool is_blank() const
    {
        return m_type == ATNNodeType::Blank;
    }
    bool is_whitespace() const
    {
        return m_type == ATNNodeType::WhiteSpace;
    }
    const NFA<TCHAR>& get_nfa() const
    {
        return m_nfa;
    }
    const std::basic_string<TCHAR>& get_literal() const
    {
        return m_literal;
    }
    const Identifier& get_invoke() const
    {
        return m_invoke;
    }
    const Identifier& get_submachine() const
    {
        return m_invoke;
    }
	int get_id() const
	{
		return m_localid;
	}
};

template<typename TCHAR> class ATNMachine
{
    friend class ATNPrinter<TCHAR>;

    std::vector<ATNNode<TCHAR> > m_nodes;
    int m_globalid;
	
	void parse_atom(Stream& stream);
	void parse_sequence(Stream& stream);
	void parse_selection(Stream& stream);
public:
    void parse(Stream& stream);
	bool verify_invocations(const std::unordered_map<Identifier, ATNMachine<TCHAR> >& network) const;
    int add_node(int from, int priority = 0)
    {
        m_nodes[from].add_transition(m_nodes.size(), priority);
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
    int get_node_num() const
    {
        return m_nodes.size();
    }
    ATNMachine(int id, Stream& stream)
        : m_globalid(id)
    {
        parse(stream);
    }
    ATNMachine(ATNMachine&& atn)
        : m_nodes(std::move(atn.m_nodes)), m_globalid(atn.m_globalid)
    {
    }
	ATNMachine(int id)
		: m_globalid(id)
	{
	}
    virtual ~ATNMachine()
    {
    }
    typename std::vector<ATNNode<TCHAR> >::const_iterator begin() const
    {
        return m_nodes.cbegin();
    }
    typename std::vector<ATNNode<TCHAR> >::const_iterator end() const
    {
        return m_nodes.cend();
    }
    int get_unique_id() const
    {
        return m_globalid;
    }
    int count_incoming_edges(int index, int& where) const
    {
        int count = 0;
        for (int i = 0; i < m_nodes.size(); i++)
        {
            for (const auto& t : m_nodes.at(i).get_transitions())
            {
                if (t.dest() == index)
                {
                    where = i;
                    count++;
                    //break;  //Overlapping edges should be counted as one.
                }
            }
        }
        if (count != 1) where = -1;
        return count;
    }
    void filter_nodes(const std::vector<bool>& mask)
    {
        std::vector<ATNNode<TCHAR> > nodes;
        std::vector<int> index_map(m_nodes.size());

        assert(m_nodes.size() == mask.size());

        for (int src_index = 0, dest_index = 0; src_index < m_nodes.size(); src_index++)
        {
            if (mask[src_index])
            {
                std::vector<ATNTransition<TCHAR> > filtered;
                for (const auto& t : m_nodes.at(src_index).get_transitions())
                {
                    if (mask[t.dest()])
                    {
                        filtered.push_back(t);
                    }
                }
                nodes.push_back(ATNNode<TCHAR>(m_nodes.at(src_index), std::move(filtered)));
                index_map[src_index] = dest_index;
                dest_index++;
            }
            else
            {
                index_map[src_index] = -1;
            }
        }

        for (auto& n : nodes)
        {
            for (auto& t : n.get_transitions())
            {
                assert(index_map[t.dest()] >= 0);

                t.dest(index_map[t.dest()]);
            }
        }
        m_nodes = std::move(nodes);
    }
    void drop_passthrough_nodes()
    {
        std::vector<bool> mask(m_nodes.size(), true);

        for (int i = 1; i < m_nodes.size() - 1; i++)
        {
            const ATNNode<TCHAR>& node = m_nodes.at(i);
            if (node.type() == ATNNodeType::Blank)
            {
                int where = -1;
                int count = count_incoming_edges(i, where);

                if (count == 1)
                {
                    assert(where >= 0);
                    ATNNode<TCHAR>& origin = m_nodes.at(where);
                    for (const auto& t : node.get_transitions())
                    {
                        origin.add_transition(t);
                    }
                    mask[i] = false;
                }
            }
        }

        filter_nodes(mask);
    }
private:
	/*void expand(int& off, const ATNMachine<TCHAR>& src, const std::unordered_map<Identifier, ATNMachine<TCHAR> >& fragments)
	{
		for (const auto& node : src.m_nodes)
		{
			if (node.type() == ATNNodeType::Nonterminal)
			{
				auto f = fragments.find(node.get_invoke());
				if (f == fragments.end())
				{
					m_nodes.push_back(node.offset(off++));
				}
				else
				{
					expand(off, f->second, fragments);

					
				}
			}
			else
			{
				m_nodes.push_back(node.offset(off++));
			}
		}
	}
public:
	void expand(const std::unordered_map<Identifier, ATNMachine<TCHAR> >& fragments)
	{
		
		std::vector<int> node_map(m_nodes.size());

		for (int i = 0; i < m_nodes.size(); i++)
		{
			if (m_nodes[i].type() == ATNNodeType::Nonterminal)
			{
				auto f = fragments.find(m_nodes[i].get_invoke());

				if (f != fragments.end())
				{
					if (f->second.)
				}
			}
			else
			{
				new_nodes.push_back(m_nodes[i]);
				node_map[i] = new_nodes.size() - 1;
			}
		}
	}*/
};

template<typename TCHAR>
class ATNPrinter
{
    const std::unordered_map<Identifier, ATNMachine<TCHAR> >& m_networks;
    std::vector<Identifier> m_stack;
    int m_counter, m_maxdepth;
private:
	std::pair<std::wstring, std::wstring> print_atn(std::wostream& os, int index, const Identifier& key)
	{
		std::wstring prefix = key + std::to_wstring(m_counter++);

		os << L"subgraph cluster_" << prefix << L" {" << std::endl;

		const ATNMachine<TCHAR>& atn = m_networks.at(key);

		std::vector<std::pair<std::wstring, std::wstring> > entry_exit_nodes(atn.m_nodes.size());

		for (unsigned int i = 0; i < atn.m_nodes.size(); i++)
		{
			const ATNNode<TCHAR>& node = atn.m_nodes[i];
			std::wstring node_name = prefix + L"_N" + std::to_wstring(i);

			if (node.is_nonterminal())
			{
				if (m_stack.size() < (size_t)m_maxdepth && std::find(m_stack.begin(), m_stack.end(), node.m_invoke) == m_stack.end())
				{
					m_stack.push_back(node.m_invoke);
					entry_exit_nodes[i] = print_atn(os, i, node.m_invoke);
					m_stack.pop_back();
				}
				else
				{
					entry_exit_nodes[i] = node.get_entry_exit_wide(node_name);
					node.print(os, i, node_name);
				}
			}
			else
			{
				entry_exit_nodes[i] = node.get_entry_exit_wide(node_name);
				node.print(os, i, node_name);
			}
		}

		for (unsigned int i = 0; i < atn.m_nodes.size(); i++)
		{
			const ATNNode<TCHAR>& node = atn.m_nodes[i];
			const std::wstring& exit = entry_exit_nodes[i].second;
			for (const auto& t : node.m_transitions)
			{
				const std::wstring& entry = entry_exit_nodes[t.dest()].first;

				os << exit << L" -> " << entry << L" [ label=\"";
				os << t;
				os << L"\" ];" << std::endl;
			}
		}

		os << L"label = \"[" << index << L"]" << key << L"\";" << std::endl;
		os << L"}" << std::endl;

		std::wstring entry_node = prefix + L"_N0";
		std::wstring exit_node = prefix + L"_N" + std::to_wstring(atn.m_nodes.size() - 1);

		return std::pair<std::wstring, std::wstring>(entry_node, exit_node);
	}
public:
    ATNPrinter(const std::unordered_map<Identifier, ATNMachine<TCHAR> >& networks, int maxdepth)
        : m_networks(networks), m_counter(0), m_maxdepth(maxdepth)
    {
    }
    virtual ~ATNPrinter()
    {
    }
	void print(std::wostream& os, const Identifier& key)
	{
		print_atn(os, 0, key);
	}
};
}
