#pragma once

namespace Centaurus
{
/*class EncodingException : public std::exception
{
    std::string m_msg;
public:
    EncodingException(const std::string& enc_from, const std::string& enc_to, const std::string& enc_str) noexcept
    {
        std::ostringstream os;
        os << "Error converting from " << enc_from << " to " << enc_to;
        m_msg = os.str();
    }
    EncodingException(const std::string& enc_from, const std::string& enc_to, int codepoint) noexcept
    {
        std::ostringstream os;
        os << "Error converting from " << enc_from << " to " << enc_to << " : 0x" << std::hex << codepoint;
        m_msg = os.str();
    }
    virtual ~EncodingException() noexcept = default;
    const char *what() const noexcept
    {
        return m_msg.c_str();
    }
};*/
class Encoder
{
    const std::codecvt<wchar_t, char, std::mbstate_t>& m_forward_cvt;
public:
    Encoder()
        : m_forward_cvt(std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t> >(std::locale()))
    {
    }
    std::wstring mbstowcs(const std::string& external)
    {
        std::mbstate_t mb{};
        std::wstring internal(external.size(), '\0');

        const char *from_next;
        wchar_t *to_next;

        m_forward_cvt.in(mb, &external[0], &external[external.size()], from_next, &internal[0], &internal[internal.size()], to_next);

        internal.resize(to_next - &internal[0]);

        return internal;
    }
    std::string wcstombs(const std::wstring& internal)
    {
        std::mbstate_t mb{};
        std::string external(internal.size() * m_forward_cvt.max_length(), '\0');

        const wchar_t *from_next;
        char *to_next;

        m_forward_cvt.out(mb, &internal[0], &internal[internal.size()], from_next, &external[0], &external[external.size()], to_next);

        external.resize(to_next - &external[0]);

        return external;
    }
};
}
