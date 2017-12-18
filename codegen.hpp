#pragma once

#include <iostream>
#include <string>
#include <type_traits>

#include <assert.h>

#include "catn2.hpp"
#include "ldfa.hpp"
#include "charclass.hpp"
#include "dfa.hpp"

namespace Centaurus
{
class CodeFormatterBase;
static void NewLine(CodeFormatterBase&);
class CodeFormatterBase
{
    friend void NewLine(CodeFormatterBase&);

    std::ofstream m_ofs;
    int m_indlevel;
    bool m_lineflag;
public:
    CodeFormatterBase(const std::string& path)
        : m_ofs(path), m_indlevel(0), m_lineflag(true)
    {
    }
    virtual ~CodeFormatterBase()
    {
    }
    void indent(int n)
    {
        if (m_indlevel + n >= 0)
            m_indlevel += n;
    }
    template<typename T> CodeFormatterBase& operator<<(const T& obj)
    {
        if (m_lineflag)
        {
            for (int i = 0; i < m_indlevel; i++)
            {
                m_ofs << '\t';
            }
            m_lineflag = false;
        }
        m_ofs << obj;
        return *this;
    }
    CodeFormatterBase& operator<<(void (*pf)(CodeFormatterBase&))
    {
        pf(*this);
        return *this;
    }
};
static void NewLine(CodeFormatterBase& fmt)
{
    fmt.m_ofs << std::endl;
    fmt.m_lineflag = true;
}
class IndentedBlock
{
    CodeFormatterBase& m_fmt;
public:
    IndentedBlock(CodeFormatterBase& fmt)
        : m_fmt(fmt)
    {
        m_fmt << '{' << NewLine;
        m_fmt.indent(+1);
    }
    ~IndentedBlock()
    {
        m_fmt.indent(-1);
        m_fmt << '}' << NewLine;
    }
};
template<typename TCHAR, class GeneratorImpl, class FormatterImpl>
class CodeGeneratorBase
{
protected:
    FormatterImpl m_fmt;
    int m_temp_counter;
protected:
    std::string get_temp_varname(const char *prefix)
    {
        std::string ret = std::string(prefix) + std::to_string(m_temp_counter);
        m_temp_counter++;
        return ret;
    }
public:
    CodeGeneratorBase(const std::string& path)
        : m_fmt(path), m_temp_counter(0)
    {
    }
    virtual ~CodeGeneratorBase()
    {
    }
    virtual void operator()(const CATNMachine<TCHAR>& machine) = 0;
    virtual void operator()(const LookaheadDFA<TCHAR>& ldfa, const std::wstring& name, int index) = 0;
};
template<typename TCHAR>
class CppCodeFormatter : public CodeFormatterBase
{
public:
    CppCodeFormatter()
    {
    }
    virtual ~CppCodeFormatter()
    {
    }
};
class CppParseFunc
{
    CodeFormatterBase& m_fmt;
public:
    CppParseFunc(CodeFormatterBase& fmt, const std::string& name)
        : m_fmt(fmt)
    {
        m_fmt << "mysval parse_" << name << "(mystream& stream)" << NewLine;
        m_fmt << "{" << NewLine;
        m_fmt.indent(+1);
    }
    virtual ~CppParseFunc()
    {
        m_fmt.indent(-1);
        m_fmt << "}" << NewLine;
    }
};
class CppDecisionFunc
{
public:
    CppDecisionFunc()
    {
    }
    virtual ~CppDecisionFunc()
    {
    }
};
template<typename TCHAR>
class CppCodeGenerator : public CodeGeneratorBase<TCHAR, CppCodeGenerator<TCHAR> , CppCodeFormatter<TCHAR> >
{
    using base = CodeGeneratorBase<TCHAR, CppCodeGenerator<TCHAR>, CppCodeFormatter<TCHAR> >;
    using base::m_fmt;

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
    const char *get_char_prefix() const
    {
        if (std::is_same<TCHAR, char>::value)
            return "";
        else if (std::is_same<TCHAR, char16_t>::value)
            return "u";
        else if (std::is_same<TCHAR, char32_t>::value)
            return "U";
        else if (std::is_same<TCHAR, wchar_t>::value)
            return "L";
        else
            return "u8";
    }
    char get_hex_digit(int hex) const
    {
        return (hex & 0xF) < 10 ? (hex & 0xF) + '0' : (hex & 0xF) - 10 + 'A';
    }
    void print_char_code(TCHAR ch)
    {
        for (int i = 0; i < sizeof(TCHAR); i++)
        {
            m_fmt << get_hex_digit(ch >> ((sizeof(TCHAR) - 1 - i) * 8 + 4));
            m_fmt << get_hex_digit(ch >> ((sizeof(TCHAR) - 1 - i) * 8));
        }
    }
    void print_char_literal(TCHAR ch)
    {
        m_fmt << get_char_prefix() << "'\\x";
        print_char_code(ch);
        m_fmt << "'";
    }
    void declare_char_var(const char *name)
    {
        m_fmt << get_chartype() << " " << name << NewLine;
    }
    void declare_string_literal(const char *name, const TCHAR *seq)
    {
        m_fmt << "const " << get_chartype() << " *" << name << " = ";
        m_fmt << get_char_prefix() << "\"";
        for (; *seq != 0; seq++)
        {
            print_char_code(*seq);
        }
        m_fmt << "\";" << NewLine;
    }
    void print_charclass_cond(const CharClass<TCHAR>& cc, const char *var)
    {
        for (auto i = cc.cbegin(); i != cc.cend();)
        {
            assert(i->start() < i->end());

            if (i->start() + 1 == i->end())
            {
                m_fmt << "(" << var << " == ";
                print_char_literal(i->start());
                m_fmt << ")";
            }
            else
            {
                m_fmt << "(";
                print_char_literal(i->start());
                m_fmt << " <= " << var << " && " << var << " < ";
                print_char_literal(i->end());
                m_fmt << ")";
            }

            if (++i != cc.cend())
            {
                m_fmt << " || ";
            }
        }
    }
public:
    CppCodeGenerator(const std::string& path)
        : base(path)
    {
    }
    void match_single_char(TCHAR ch)
    {
        IndentedBlock _b(m_fmt);
        {
            declare_char_var("ch");

            m_fmt << "ch = stream.get();" << NewLine;
            m_fmt << "if (ch != ";
            print_char_literal(ch);
            m_fmt << ')' << NewLine;
            m_fmt << "\tthrow stream.unexpected(ch);" << NewLine;
        }
    }
    void match_string(const TCHAR *seq)
    {
        IndentedBlock _b0(m_fmt);
        {
            declare_string_literal("pat", seq);

            m_fmt << NewLine;
            m_fmt << "for (int i = 0; i < " << target_strlen<TCHAR>(seq) << "; i++)" << NewLine;
            
            IndentedBlock _b1(m_fmt);
            {
                declare_char_var("ch");

                m_fmt << "ch = stream.get();" << NewLine;
                m_fmt << "if (ch != pat[i])" << NewLine;
                m_fmt << "\tthrow stream.unexpected(ch);" << NewLine;
            }
        }
    }
    void match_regex_r(const DFA<TCHAR>& dfa, int index)
    {
        //if (
    }
    void match_regex(const NFA<TCHAR>& nfa)
    {
        DFA<TCHAR> dfa(nfa);

        IndentedBlock _b(m_fmt);
        {
            match_regex_r(dfa, 0);
        }
    }
    virtual ~CppCodeGenerator()
    {
    }
};
}
