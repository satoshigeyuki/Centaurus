#pragma once

#include <exception>

namespace Centaurus
{
class SimpleException : public std::exception
{
    std::string m_msg;
public:
    SimpleException(const std::string& msg) noexcept
        : m_msg(msg)
    {
    }
    virtual ~SimpleException()
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
    UnexpectedException(char16_t ch) noexcept
    {
        std::basic_ostringstream<char16_t> stream;

        if (ch == EOF)
            stream << u"Unexpected EOF" << std::endl;
        else
            stream << u"Unexpected character " << ch << std::endl;

        std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t > converter;

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
public:
    ReservedCharException() noexcept
    {
    }
    virtual ~ReservedCharException()
    {
    }
    const char *what() const noexcept
    {
        return "Reserved character U+FFFF is not allowed here.";
    }
};
}
