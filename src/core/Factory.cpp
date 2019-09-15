#include "Grammar.hpp"
#include "ATNMachine.hpp"
#include "wchar.h"

class IFactory
{
public:
    virtual IGrammar *make_grammar() = 0;
    virtual ~IFactory() = default;
};

template<typename TCHAR>
class Factory : public IFactory
{
public:
    Factory() {}
    virtual ~Factory() {}
    virtual IGrammar *make_grammar() const
    {
        return new Grammar<TCHAR>();
    }
    virtual IATNMachine *make_machine() const
    {
        return new ATNMachine<TCHAR>();
    }
};

template class Factory<char>;
template class Factory<wchar_t>;

static int wcscmpci(const wchar_t *a, const wchar_t *b)
{
    for (; *a != L'\0' && *b != L'\0'; a++, b++)
    {
        if (*a < *b) return -1;
        if (*a > *b) return 1;
    }
    if (*a < *b) return -1;
    if (*a > *b) return 1;
    return 0;
}

extern "C" IFactory *FactoryCreate(const wchar_t *charset)
{
    if (!wcscmpci(charset, L"ascii"))
        return new Factory<char>();
    else if (!wcscmpci(charset, L"unicode"))
        return new Factory<wchar_t>();
    else
        return NULL;
}

extern "C" void FactoryDestroy(IFactory *factory)
{
    delete factory;
}
