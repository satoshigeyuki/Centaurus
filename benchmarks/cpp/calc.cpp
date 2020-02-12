#include "Context.hpp"

using namespace Centaurus;

struct Number
{
    int num;
    Number(int num) : num(num) {}
    Number(const std::string& str) : num(std::stoi(str)) {}
};
static volatile int result;

void *parse_INPUT(const SymbolContext<char>& ctx)
{
    result = ctx.value<Number>(1)->num;
    return nullptr;
}
void *parse_EXPR(const SymbolContext<char>& ctx)
{
    if (ctx.len() == 1)
        return ctx.value<Number>(1);
    else
        return new Number{ctx.value<Number>(1)->num + ctx.value<Number>(2)->num};
}
void *parse_TERM(const SymbolContext<char>& ctx)
{
    if (ctx.len() == 1)
        return ctx.value<Number>(1);
    else
        return new Number{ctx.value<Number>(1)->num * ctx.value<Number>(2)->num};
}
void *parse_FACT(const SymbolContext<char>& ctx)
{
    if (ctx.start()[0] == '(')
        return ctx.value<Number>(1);
    else
        return new Number(ctx.read());
}

int main(int argc, const char *argv[])
{
    Context<char> context{"../grammars/calc.cgr"};

    context.attach(L"INPUT", parse_INPUT);
    context.attach(L"EXPR", parse_EXPR);
    context.attach(L"TERM", parse_TERM);
    context.attach(L"FACT", parse_FACT);

    context.parse(argv[1], 1);

    printf("Result = %d\n", result);

    return 0;
}