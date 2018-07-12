#pragma once

#include <vector>
#include <string>
#include <unordered_map>

namespace Centaurus
{
template<typename TCHAR>
class WideATNPrinter
{
	const std::unordered_map<std::basic_string<TCHAR>, ATNMachine<TCHAR> >& m_networks;
	std::vector<std::basic_string<TCHAR> > m_stack;
	int m_counter, m_maxdepth;
private:
	std::pair<std::basic_string<TCHAR>, std::basic_string<TCHAR> > print_atn(std::wostream& os, const std::basic_string<TCHAR>& key)
	{
		std::wstring prefix = key + std::to_wstring(m_counter++);

		os << L"subgraph cluster_" << prefix << L" {" << std::endl;

		const ATNMachine<TCHAR>& atn = m_networks.at(key);

		std::vector<std::pair<std::basic_string<TCHAR>, std::basic_string<TCHAR> > > entry_exit_nodes(atn.m_nodes.size());

		for (unsigned int i = 0; i < atn.m_nodes.size(); i++)
		{
			const ATNNode<TCHAR>& node = atn.m_nodes[i];
			std::wstring node_name = prefix + L"_N" + std::to_wstring(i);

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
					node.print_wide(os, node_name);
				}
			}
			else
			{
				entry_exit_nodes[i] = node.get_entry_exit(node_name);
				node.print_wide(os, node_name);
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
	ATNPrinter(const std::unordered_map<Identifier, ATNMachine<TCHAR> >& networks, int maxdepth)
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


template<typename TCHAR>
class GenericGrammar
{
	std::unordered_map<std::basic_string<TCHAR>, ATNMachine<TCHAR> > m_networks;
	std::vector<std::basic_string<TCHAR> > m_identifiers;
	std::basic_string<TCHAR> m_root_id;
public:
	GenericGrammar()
	{
	}
	GenericGrammar(GenericGrammar<TCHAR>&& old)
		: m_networks(std::move(old.m_networks)), m_identifiers(std::move(old.m_identifiers)), m_root_id(old.m_root_id)
	{
	}
	virtual ~GenericGrammar()
	{
	}
	void add_rule(const std::basic_string<TCHAR>& lhs, const std::basic_string<TCHAR>& rhs)
	{
		if (m_root_id.empty())
			m_root_id.assign(lhs);

		int machine_index = m_networks.size() + 1;

		m_networks.emplace(lhs, ATNMachine<TCHAR>(machine_index, rhs));
		
		m_identifiers.push_back(lhs);
	}
	const ATNMachine<TCHAR>& operator[](const std::basic_string<TCHAR>& id) const
	{
		return m_networks.at(id);
	}
	const std::unordered_map<std::basic_string<TCHAR>, ATNMachine<TCHAR> >& get_machines() const
	{
		return m_networks;
	}
	int get_machine_num() const
	{
		return m_networks.size();
	}
	const std::basic_string<TCHAR>& get_root_id() const
	{
		return m_root_id;
	}
	typename std::unordered_map<std::basic_string<TCHAR>, ATNMachine<TCHAR> >::const_iterator begin() const
	{
		return m_networks.cbegin();
	}
	typename std::unordered_map<std::basic_string<TCHAR>, ATNMachine<TCHAR> >::const_iterator end() const
	{
		return m_networks.cend();
	}
	int get_machine_id(const std::basic_string<TCHAR>& id) const
	{
		return m_networks.at(id).get_unique_id();
	}
	const std::basic_string<TCHAR>& lookup_id(int index) const
	{
		return m_identifiers.at(index);
	}
};
using WideGrammar = GenericGrammar<wchar_t>;
}