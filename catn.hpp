#pragma once

namespace Centaurus
{
template<typename TCHAR> class CATN : public ATN<TCHAR>
{
    const std::unordered_map<Identifier, ATN<TCHAR> >& m_networks;
public:
    CATN(const std::unordered_map<Identifier, ATN<TCHAR> >& networks, const Identifier& root)
        : m_networks(networks)
    {
    }
    virtual ~CATN()
    {
    }
};
}
