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
}
