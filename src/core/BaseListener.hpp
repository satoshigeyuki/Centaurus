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
struct SymbolEntry
{
	long id;
	long start, end;
	SymbolEntry(int id, long start, long end)
		: id(id), start(start), end(end)
	{
	}
};
}
