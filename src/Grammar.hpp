#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <assert.h>

#include "Identifier.hpp"
#include "ATN.hpp"
#include "DFA.hpp"
#include "Platform.hpp"

namespace Centaurus
{
typedef void (CENTAURUS_CALLBACK * EnumMachinesCallback)(const wchar_t *name, int id);

class IGrammar
{
public:
	IGrammar() {}
	virtual ~IGrammar() {}

	virtual void enum_machines(EnumMachinesCallback callback) const = 0;
	virtual void print_grammar(std::wostream& ofs, int maxdepth, bool optimize_flag = false) {}

    virtual void print_nfa(std::wostream& ofs, const ATNPath& path) const {}
    virtual void print_dfa(std::wostream& ofs, const ATNPath& path, bool optimize_flag = false) const {}
    virtual void print_ldfa(std::wostream& ofs, const ATNPath& path) const {}
    virtual void print_catn(std::wostream& ofs, const Identifier& id) const {}

    virtual void optimize() {}
};
class GrammarOptions : public std::unordered_map<Identifier, std::wstring>
{
    void parse(Stream& stream);
public:
    GrammarOptions()
    {
    }
    GrammarOptions(Stream& stream)
    {
        parse(stream);
    }
    virtual ~GrammarOptions()
    {
    }
};
template<typename TCHAR> class Grammar : public IGrammar
{
    std::unordered_map<Identifier, ATNMachine<TCHAR> > m_networks;
    std::vector<Identifier> m_identifiers;
    Identifier m_root_id;
	Identifier m_grammar_name;
    GrammarOptions m_options;
public:
    void parse(Stream& stream);
    const ATNNode<TCHAR>& resolve(const ATNPath& path) const
    {
        return m_networks.at(path.leaf_id()).get_node(path.leaf_index());
    }
public:
    Grammar(Stream& stream)
    {
        parse(stream);
    }
    Grammar()
    {
    }
    Grammar(const char *path)
    {
        std::wifstream grammar_file(path);

        std::wstring grammar_str(std::istreambuf_iterator<wchar_t>(grammar_file), {});

        Stream stream(std::move(grammar_str));

        parse(stream);
    }
    Grammar(Grammar&& old)
        : m_networks(std::move(old.m_networks)), m_root_id(old.m_root_id)
    {
    }
    virtual ~Grammar()
    {
    }
	void print(std::wostream& os, const Identifier& key, int maxdepth = 3) const
	{
		os << L"digraph " << key << L" {" << std::endl;
		os << L"rankdir=\"LR\";" << std::endl;
		os << L"graph [ charset=\"UTF-8\" ];" << std::endl;
		os << L"node [ style=\"solid,filled\" ];" << std::endl;
		os << L"edge [ style=\"solid\" ];" << std::endl;

		ATNPrinter<TCHAR> printer(m_networks, maxdepth);

		printer.print(os, key);

		os << L"}" << std::endl;
	}
	virtual void print_grammar(std::wostream& os, int maxdepth, bool optimize_flag) override
	{
        if (optimize_flag)
        {
            optimize();
        }

		print(os, get_root_id(), maxdepth);
	}
    virtual void print_nfa(std::wostream& os, const ATNPath& path) const override
    {
        const ATNNode<TCHAR>& node = resolve(path);

        if (node.type() != ATNNodeType::RegularTerminal)
        {
            throw SimpleException("The specified node is not an NFA.");
        }
        else
        {
            const NFA<TCHAR>& nfa = node.get_nfa();

            nfa.print(os, path.leaf_id().str());
        }
    }
    virtual void print_dfa(std::wostream& os, const ATNPath& path, bool optimize_flag) const override
    {
        const ATNNode<TCHAR>& node = resolve(path);

        if (node.type() != ATNNodeType::RegularTerminal)
        {
            throw SimpleException("The specified node is not an NFA.");
        }
        else
        {
            const NFA<TCHAR>& nfa = node.get_nfa();

            DFA<TCHAR> dfa(nfa, optimize_flag);

            dfa.print(os, path.leaf_id().str());
        }
    }
    virtual void print_ldfa(std::wostream& os, const ATNPath& path) const override;
    virtual void print_catn(std::wostream& os, const Identifier& id) const override;
    const ATNMachine<TCHAR>& operator[](const Identifier& id) const
    {
        return m_networks.at(id);
    }
    const std::unordered_map<Identifier, ATNMachine<TCHAR> >& get_machines() const
    {
        return m_networks;
    }
    int get_machine_num() const
    {
        return m_networks.size();
    }
    const Identifier& get_root_id() const
    {
        return m_root_id;
    }
    typename std::unordered_map<Identifier, ATNMachine<TCHAR> >::const_iterator begin() const
    {
        return m_networks.cbegin();
    }
    typename std::unordered_map<Identifier, ATNMachine<TCHAR> >::const_iterator end() const
    {
        return m_networks.cend();
    }
	int get_machine_id(const Identifier& id) const
	{
		return m_networks.at(id).get_unique_id();
	}
    const Identifier& lookup_id(int index) const
    {
        return m_identifiers.at(index);
    }
	virtual void enum_machines(EnumMachinesCallback callback) const override
	{
		for (int i = 0; i < m_identifiers.size(); i++)
		{
			callback(m_identifiers[i].str().c_str(), i + 1);
		}
	}
    virtual void optimize() override
    {
        for (auto& p : m_networks)
        {
            p.second.drop_passthrough_nodes();
        }
    }
};
}
