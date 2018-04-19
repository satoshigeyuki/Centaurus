#pragma once

namespace Centaurus
{
class BaseListener
{
public:
    BaseListener() {}
    virtual ~BaseListener() {}
    virtual void *feed_callback() { return NULL; }
    virtual void terminal_callback(int id, const void *start, const void *end) {}
    virtual const void *nonterminal_callback(int id, const void *input) { return NULL; }
};
}