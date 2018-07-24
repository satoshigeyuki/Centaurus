#pragma once

#include <iostream>
#include <string>
#include <unordered_map>

#include "Identifier.hpp"
#include "ATN.hpp"

namespace Centaurus
{
/*class AlienCode
{
protected:
    std::u16string m_str;
public:
    AlienCode()
    {
    }
    virtual ~AlienCode()
    {
    }
};
class CppAlienCode : public AlienCode
{
    using AlienCode::m_str;
private:
    void parse_literal_string(Stream& stream)
    {
        char16_t ch = stream.get();
        for (; ch != u'\0'; ch = stream.get())
        {
            if (ch == u'"')
                return;
        }
        throw stream.unexpected(EOF);
    }
    void parse_literal_character(Stream& stream)
    {
        char16_t ch = stream.get();
        if (ch == '\\')
            ch = stream.get();
        ch = stream.get();
        if (ch != '\'')
            throw stream.unexpected(ch);
    }
    void parse(Stream& stream)
    {
        while (true)
        {
            char16_t ch = stream.after_whitespace();

            switch (ch)
            {
            case u'"':
                parse_literal_string(stream);
                break;
            case u'\'':
                parse_literal_character(stream);
                break;
            }
        }
    }
public:
    CppAlienCode(Stream& stream)
    {
        Stream::Sentry begin = stream.sentry();

        parse(stream);

        m_str = stream.cut(begin);
    }
    virtual ~CppAlienCode() = default;
};*/
typedef void (__cdecl * EnumMachinesCallback)(const wchar_t *name, int id);

class IGrammar
{
public:
	IGrammar() {}
	virtual ~IGrammar() {}

	virtual void enum_machines(EnumMachinesCallback callback) = 0;
	virtual void print(std::wostream& ofs, int maxdepth) {}
};
template<typename TCHAR> class Grammar : public IGrammar
{
    std::unordered_map<Identifier, ATNMachine<TCHAR> > m_networks;
    std::vector<Identifier> m_identifiers;
    Identifier m_root_id;
public:
    void parse(Stream& stream)
    {
        int machine_id = 1;

        while (1)
        {
            wchar_t ch = stream.skip_whitespace();

            if (ch == L'\0')
                break;

            if (machine_id >= 65536)
                throw stream.toomany(65535);

            Identifier id(stream);
            ATNMachine<TCHAR> atn(machine_id++, stream);

            if (m_root_id.str().empty())
                m_root_id = id;

            m_networks.insert(std::pair<Identifier, ATNMachine<TCHAR> >(id, std::move(atn)));

            m_identifiers.push_back(id);

            //m_networks.emplace(Identifier(stream), ATN<TCHAR>(stream));
        }
    }
public:
    Grammar(Stream& stream)
    {
        parse(stream);
    }
    Grammar()
    {
    }
    Grammar(Grammar&& old)
        : m_networks(std::move(old.m_networks)), m_root_id(old.m_root_id)
    {
    }
    virtual ~Grammar()
    {
    }
    void print(std::ostream& os, const Identifier& key, int maxdepth = 3)
    {
        os << "digraph " << key.narrow() << " {" << std::endl;
        os << "rankdir=\"LR\";" << std::endl;
        os << "graph [ charset=\"UTF-8\" ];" << std::endl;
        os << "node [ style=\"solid,filled\" ];" << std::endl;
        os << "edge [ style=\"solid\" ];" << std::endl;

        ATNPrinter<TCHAR> printer(m_networks, maxdepth);

        printer.print(os, key);

        os << "}" << std::endl;
    }
	void print(std::wostream& os, const Identifier& key, int maxdepth = 3)
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
	virtual void print(std::wostream& os, int maxdepth) override
	{
		print(os, get_root_id(), maxdepth);
	}
    /*CompositeATN<TCHAR> build_catn() const
    {
        CompositeATN<TCHAR> catn(m_networks, m_root_id);

        return catn;
    }*/
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
	virtual void enum_machines(EnumMachinesCallback callback) override
	{
		for (int i = 0; i < m_identifiers.size(); i++)
		{
			callback(m_identifiers[i].str().c_str(), i + 1);
		}
	}
};
}
