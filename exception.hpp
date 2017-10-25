#pragma once

#include <exception>

namespace Centaur
{
class Exception : public std::exception
{
    std::string m_msg;
public:
    Exception(const std::string& msg) noexcept
        : m_msg(msg)
    {
    }
    virtual ~Exception()
    {
    }
    const char *what() const noexcept
    {
        return m_msg.c_str();
    }
};
class UnexpectedException : public std::exception
{
    std::string msg;
public:
    UnexpectedException(wchar_t ch) noexcept
    {
        std::wostringstream stream;

        if (ch == EOF)
            stream << L"Unexpected EOF" << std::endl;
        else
            stream << L"Unexpected character " << ch << std::endl;

        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > converter;

        msg = converter.to_bytes(stream.str());
    }
    virtual ~UnexpectedException()
    {
    }
    const char *what() const noexcept
    {
        return msg.c_str();
    }
};
class ReservedCharException : public std::exception
{
    std::string msg;
public:
    ReservedCharException() noexcept
    {
        msg = "Reserved character U+FFFF is not allowed here.";
    }
    virtual ~UnexpectedException()
    {
    }
    const char *what() const noexcept
    {
        return msg.c_str();
    }
};
}
