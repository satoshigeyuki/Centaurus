#pragma once

#include <vector>
#include <string>
#include <unordered_map>

namespace Centaurus
{
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