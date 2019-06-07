#include "Stage1Runner.hpp"
#include "Stage2Runner.hpp"
#include "Stage3Runner.hpp"

namespace Centaurus
{
template<typename TCHAR>
class SymbolContext;
template<typename TCHAR>
using CppReductionCallback = void *(*)(const SymbolContext<TCHAR>& ctx);
template<typename TCHAR>
struct ParseContext
{
    std::vector<CppReductionCallback<TCHAR> >& m_callbacks;
    const void *m_window;
    ParseContext(std::vector<CppReductionCallback<TCHAR> >& callbacks, const void *window)
        : m_callbacks(callbacks), m_window(window)
    {
    }
};
template<typename TCHAR>
class SymbolContext
{
    const void *m_window;
    const SymbolEntry *m_symbols;
    int m_symbol_num;
public:
    SymbolContext(const void *window, const SymbolEntry *symbols, int symbol_num)
        : m_window(window), m_symbols(symbols), m_symbol_num(symbol_num)
    {
    }
    std::basic_string<TCHAR> read(int index = 0) const
    {
        if (index < m_symbol_num)
        {
            long start = m_symbols[index].start;
            long end = m_symbols[index].end;
            const TCHAR *p = (const TCHAR *)m_window + start;
            long len = end - start;
            return std::basic_string<TCHAR>(p, len);
        }
        return std::basic_string<TCHAR>();
    }
    const TCHAR *start(int index = 0) const
    {
        if (index < m_symbol_num)
        {
            long start = m_symbols[index].start;
            return (const TCHAR *)m_window + start;
        }
        return NULL;
    }
    const TCHAR *end(int index = 0) const
    {
        if (index < m_symbol_num)
        {
            long end = m_symbols[index].end;
            return (const TCHAR *)m_window + end;
        }
        return NULL;
    }
    const int len(int index = 0) const
    {
        if (index < m_symbol_num)
        {
            long start = m_symbols[index].start;
            long end = m_symbols[index].end;
            return end - start;
        }
        return 0;
    }
    int count() const
    {
        return m_symbol_num - 1;
    }
    template<typename T>
    T *value(int index) const
    {
        return reinterpret_cast<T *>(m_symbols[index].key);
    }
};
template<typename TCHAR>
class Context
{
    Grammar<TCHAR> m_grammar;
    ParserEM64T<TCHAR> m_parser;
    ChaserEM64T<TCHAR> m_chaser;
    std::vector<CppReductionCallback<TCHAR> > m_callbacks;
    static long CENTAURUS_CALLBACK callback(const SymbolEntry *symbols, int symbol_num, void *context)
    {
        ParseContext<TCHAR> *ctx = (ParseContext<TCHAR> *)context;
        SymbolContext<TCHAR> sc(ctx->m_window, symbols, symbol_num);
        if (symbols[0].id < ctx->m_callbacks.size())
            if (ctx->m_callbacks[symbols[0].id] != nullptr)
                return (long)ctx->m_callbacks[symbols[0].id](sc);
        return 0;
    }
public:
    Context(const char *filename)
    {
        std::wifstream grammar_file(filename, std::ios::in);

        std::wstring wide_grammar(std::istreambuf_iterator<wchar_t>(grammar_file), {});

        Stream stream(std::move(wide_grammar));

        m_grammar.parse(stream);

        m_parser.init(m_grammar);
        m_chaser.init(m_grammar);

        m_callbacks.resize(m_grammar.get_machine_num() + 1, nullptr);
    }
    void parse(const char *input_path, int worker_num)
    {
        int pid = get_current_pid();

        ParseContext<TCHAR> context{m_callbacks, nullptr};

        std::atomic<int> reduction_count(0);
        std::vector<BaseRunner *> runners;
        runners.push_back(new Stage1Runner{ input_path, &m_parser, 8 * 1024 * 1024, worker_num * 2 });
        for (int i = 0; i < worker_num; i++)
        {
            Stage2Runner *st2 = new Stage2Runner{input_path, &m_chaser, 8 * 1024 * 1024, worker_num * 2, pid, static_cast<void *>(&context), &reduction_count};

            st2->register_listener(callback);

            runners.push_back(st2);
        }
        Stage3Runner *st3 = new Stage3Runner{ input_path, &m_chaser, 8 * 1024 * 1024, worker_num * 2, pid, static_cast<void *>(&context), &reduction_count };

        st3->register_listener(callback);

        runners.push_back(st3);

        context.m_window = st3->get_input();

        for (auto p : runners)
        {
            p->start();
        }
        for (auto p : runners)
        {
            p->wait();
        }

        std::cout << reduction_count.load() << " reductions" << std::endl;

        for (auto p : runners)
        {
            delete p;
        }
    }
    void attach(const Identifier& id, CppReductionCallback<TCHAR> callback)
    {
        int index = m_grammar.get_machine_id(id);

        m_callbacks[index] = callback;
    }
};
}
