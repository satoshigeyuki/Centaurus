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
	int id, key;
	long start, end;
	SymbolEntry(int id, long start, long end, long key = 0)
		: id(id), key(key), start(start), end(end)
	{
	}
};
}
