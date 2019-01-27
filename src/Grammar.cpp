#include "CompositeATN.hpp"
#include "LookaheadDFA.hpp"

namespace Centaurus
{
template<typename TCHAR>
void Grammar<TCHAR>::print_catn(std::wostream& os, const Identifier& id) const
{
    CompositeATN<TCHAR> catn(*this);

    catn[id].print(os, id.str());
}
template<typename TCHAR>
void Grammar<TCHAR>::print_ldfa(std::wostream& os, const ATNPath& path) const
{
    CompositeATN<TCHAR> catn(*this);

    LookaheadDFA<TCHAR> ldfa(catn, path);

    ldfa.print(os, path.leaf_id().str());
}
template<typename TCHAR>
void Grammar<TCHAR>::parse(Stream& stream)
{
    int machine_id = 1;

	std::unordered_map<Identifier, ATNMachine<TCHAR> > fragments;

    while (1)
    {
        wchar_t ch = stream.skip_whitespace();

        if (ch == L'\0')
            break;

        if (machine_id >= 65536)
            throw stream.toomany(65535);

        Identifier id(stream);

        if (id == L"grammar")
        {
            stream.skip_whitespace();

            m_grammar_name.parse(stream);

            wchar_t ch = stream.skip_whitespace();

            if (ch != L';')
                throw stream.unexpected(ch);
            stream.discard();
        }
        else if (id == L"options")
        {
            GrammarOptions options(stream);
        }
		else if (id == L"fragment")
		{
			Identifier frag_id(stream);

			ATNMachine<TCHAR> atn(-1, stream);

			fragments.emplace(frag_id, std::move(atn));
		}
        else
        {
            ATNMachine<TCHAR> atn(machine_id++, stream);

            if (m_root_id.str().empty())
                m_root_id = id;

            m_networks.insert(std::pair<Identifier, ATNMachine<TCHAR> >(id, std::move(atn)));

            m_identifiers.push_back(id);
        }

        //m_networks.emplace(Identifier(stream), ATN<TCHAR>(stream));
    }
}

template<typename TCHAR>
bool Grammar<TCHAR>::verify() const
{
    for (const auto& p : m_networks)
    {
        if (!p.second.verify_invocations(m_networks))
        {
            std::wcerr << L"Nonterminal invocation check failed at " << p.first << std::endl;
            return false;
        }
    }

    if (!CompositeATN<TCHAR>(*this).verify_decisions(m_networks))
    {
        return false;
    }

    return true;
}

void GrammarOptions::parse(Stream& stream)
{
    wchar_t ch = stream.skip_whitespace();
    if (ch != L'{')
        throw stream.unexpected(ch);
    stream.discard();

    while (true)
    {
        Identifier key(stream);

        ch = stream.skip_whitespace();
        if (ch != L'=')
            throw stream.unexpected(ch);
        stream.discard();

        Identifier value(stream);

        ch = stream.skip_whitespace();
        if (ch != L';')
            throw stream.unexpected(ch);
        stream.discard();

        emplace(key, value.str());
    }

    ch = stream.skip_whitespace();
    if (ch != L'}')
        throw stream.unexpected(ch);
    stream.discard();
}
template class Grammar<char>;
template class Grammar<unsigned char>;
template class Grammar<wchar_t>;
}
