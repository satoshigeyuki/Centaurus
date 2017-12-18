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
        std::ostringstream stream;

        if (ch == std::char_traits<char16_t>::eof())
            stream << "Unexpected EOF" << std::endl;
        else
            stream << "Unexpected character " << stream.narrow(ch, '@') << std::endl;

        msg = stream.str();
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
