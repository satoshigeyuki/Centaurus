#pragma once

#include <iostream>
#include <string>
#include <type_traits>

#include "catn2.hpp"

namespace Centaurus
{
class IndentedFileWriter : public std::ofstream
{
public:
    IndentedFileWriter(const std::string& path)
        : std::ofstream(path)
    {
    }
    virtual ~IndentedFileWriter()
    {
    }
    virtual std::ostream& put(char ch) override
    {
    }
};
template<typename TCHAR, class GeneratorImpl>
class CodeGeneratorBase
{
    IndentedFileWriter ofs;
public:
    CodeGeneratorBase(const std::string& path)
        : ofs(path)
    {
    }
    virtual ~CodeGeneratorBase()
    {
    }
    virtual void operator()(const CATNMachine<TCHAR>& machine)
    {
        
    }
};
template<typename TCHAR>
class CppCodeGenerator : public CodeGeneratorBase<TCHAR, CppCodeGenerator<TCHAR> >
{
    using CodeGeneratorBase::ofs;

    const char *get_chartype() const
    {
        if (std::is_same<TCHAR, char>::value)
            return "char";
        else if (std::is_same<TCHAR, char16_t>::value)
            return "char16_t";
        else if (std::is_same<TCHAR, char32_t>::value)
            return "char32_t";
        else if (std::is_same<TCHAR, wchar_t>::value)
            return "wchar_t";
        else
            return "int";
    }
    void declare_char_var(const char *name)
    {
        ofs << get_chartype() << " " << name << ";" << std::endl;
    }
public:
    CppCodeGenerator(const std::string& path)
        : CodeGeneratorBase(path)
    {
    }
    void match_single_char(TCHAR ch)
    {
        ofs << "{" << std::endl;
    }
    virtual ~CppCodeGenerator()
    {
    }
};
}
