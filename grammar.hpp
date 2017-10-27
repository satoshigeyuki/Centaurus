#pragma once

#include <iostream>
#include <list>

namespace Centaurus
{
std::wistream::int_type skip_chars(std::wistream& stream)
{
    std::wistream::int_type ch = stream.get();
    //for (; ch == L' ' || ch == L'\t' || ch == '\r' || 
}
class AlienCode
{
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
    void parse_multiline_comment(WideSentryStream& stream)
    {
    }
    void parse_oneline_comment(WideSentryStream& stream)
    {
    }
    void parse(WideSentryStream& stream)
    {
    }
public:
    CppAlienCode(WideSentryStream& stream)
    {
        parse(stream);
    }
    virtual ~CppAlienCode()
    {
    }
};
class Production
{
};
class Grammar
{
    std::list<Production> m_productions;
public:
    Grammar(std::wistream& stream)
    {
    }
    ~Grammar()
    {
    }
};
}
