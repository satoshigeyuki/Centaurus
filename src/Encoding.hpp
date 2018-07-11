#pragma once

#include <string>
#include <sstream>
#include <utility>
#include <errno.h>

#if defined(CENTAURUS_BUILD_LINUX)
#include <iconv.h>
#elif defined(CENTAURUS_BUILD_WINDOWS)
#define NOMINMAX
#include <Windows.h>
#endif
#include <string.h>
#include <stdlib.h>

namespace Centaurus
{
class EncodingException : public std::exception
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
};
template<typename SCHAR, typename DCHAR> class Encoder
{
#if defined(CENTAURUS_BUILD_LINUX)
    iconv_t m_iconv;
#endif
    std::string m_from, m_to;
private:
    EncodingException error(const std::basic_string<SCHAR>& str) const noexcept
    {
        return EncodingException(m_from, m_to, std::string());
    }
    EncodingException error(SCHAR sch) const noexcept
    {
        return EncodingException(m_from, m_to, std::string());
    }
public:
    Encoder(const char *from, const char *to)
        : m_from(from), m_to(to)
    {
#if defined(CENTAURUS_BUILD_LINUX)
        m_iconv = iconv_open(to, from);
#endif
    }
    ~Encoder()
    {
#if defined(CENTAURUS_BUILD_LINUX)
        iconv_close(m_iconv);
#endif
    }
    std::basic_string<DCHAR> encode_StoS(const std::basic_string<SCHAR>& str)
    {
        char *inbuf = (char *)malloc((str.size() + 1) * sizeof(SCHAR));
        char *inbuf_orig = inbuf;
        memcpy(inbuf, str.c_str(), str.size() * sizeof(SCHAR));
        size_t inbytesleft = (str.size() + 1) * sizeof(SCHAR);
        char *outbuf = (char *)malloc((str.size() + 1) * 8);
        char *outbuf_orig = outbuf;
        size_t outbytesleft = (str.size() + 1) * 8;
        size_t outbytesleft_orig = outbytesleft;
        iconv(m_iconv, NULL, NULL, NULL, NULL);
        if (iconv(m_iconv, &inbuf, &inbytesleft, &outbuf, &outbytesleft) != (size_t)-1)
        {
            std::basic_string<DCHAR> ret((const DCHAR *)outbuf_orig, (outbytesleft_orig - outbytesleft) / sizeof(DCHAR));
            free(inbuf_orig);
            free(outbuf_orig);
            return ret;
        }
        else
        {
            free(inbuf_orig);
            free(outbuf_orig);
            throw error(str);
        }
    }
    std::pair<bool, DCHAR> encode_CtoC(SCHAR ch)
    {
        char *inbuf = (char *)&ch;
        size_t inbytesleft = sizeof(SCHAR);
        DCHAR outch;
        char *outbuf = (char *)&outch;
        size_t outbytesleft = sizeof(DCHAR);
        iconv(m_iconv, NULL, NULL, NULL, NULL);
        if (iconv(m_iconv, &inbuf, &inbytesleft, &outbuf, &outbytesleft) != (size_t)-1)
        {
            return std::pair<bool, DCHAR>(true, outch);
        }
        else if (errno == E2BIG)
        {
            return std::pair<bool, DCHAR>(false, outch);
        }
        else
        {
            throw error(ch);
        }
    }
    std::basic_string<DCHAR> encode_CtoS(SCHAR ch)
    {
        char *inbuf = (char *)&ch;
        size_t inbytesleft = sizeof(SCHAR);
        char outbuf[8];
        char *outbuf_ptr = outbuf;
        size_t outbytesleft = sizeof(outbuf);
        iconv(m_iconv, NULL, NULL, NULL, NULL);
        if (iconv(m_iconv, &inbuf, &inbytesleft, &outbuf_ptr, &outbytesleft) != (size_t)-1)
        {
            return std::basic_string<DCHAR>((const DCHAR *)(outbuf), (sizeof(outbuf) - outbytesleft) / sizeof(DCHAR));
        }
        else
        {
            throw error(ch);
        }
    }
    std::pair<bool, DCHAR> encode_StoC(const std::basic_string<SCHAR>& str)
    {
        char *inbuf = (char *)malloc(str.size() * sizeof(SCHAR));
        memcpy(inbuf, str.c_str(), str.size() * sizeof(SCHAR));
        char *inbuf_orig = inbuf;
        size_t inbytesleft = str.size() * sizeof(SCHAR);
        DCHAR outbuf;
        char *outbuf_ptr = (char *)&outbuf;
        size_t outbytesleft = sizeof(outbuf);
        iconv(m_iconv, NULL, NULL, NULL, NULL);
        if (iconv(m_iconv, &inbuf, &inbytesleft, &outbuf_ptr, &outbytesleft) != (size_t)-1)
        {
            free(inbuf_orig);
            return std::pair<bool, DCHAR>(true, outbuf);
        }
        else if (errno == E2BIG)
        {
            free(inbuf_orig);
            return std::pair<bool, DCHAR>(false, 0);
        }
        else
        {
            free(inbuf_orig);
            throw error(str);
        }
    }
};
}
