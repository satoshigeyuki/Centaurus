#pragma once

#include "StageRunners.hpp"

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
  const ParseContext<TCHAR>& context;
  const SymbolEntry& symbol;
  const uint64_t * const values;
  const int num_values;
public:
  SymbolContext(const ParseContext<TCHAR>& context, const SymbolEntry& symbol, uint64_t *values, int num_values)
    : context(context), symbol(symbol), values(values), num_values(num_values)
  {
  }
  std::basic_string<TCHAR> read() const
  {
    const TCHAR *p = reinterpret_cast<const TCHAR *>(context.m_window) + symbol.start;
    return std::basic_string<TCHAR>(p, len());
  }
  const TCHAR *start() const
  {
    return reinterpret_cast<const TCHAR *>(context.m_window) + symbol.start;
  }
  const TCHAR *end() const
  {
    return reinterpret_cast<const TCHAR *>(context.m_window) + symbol.end;
  }
  const int len() const
  {
    return symbol.end - symbol.start;
  }
  int count() const
  {
    return num_values;
  }
  template<typename T>
  T *value(int i) const
  {
    return reinterpret_cast<T*>( values[i - 1]);
  }
};
template<typename TCHAR>
class Context
{
  Grammar<TCHAR> m_grammar;
  ParserEM64T<TCHAR> m_parser;
  std::vector<CppReductionCallback<TCHAR> > m_callbacks;
  static long CENTAURUS_CALLBACK callback(const SymbolEntry *symbol, uint64_t *values, int num_values, void *context)
  {
    auto& ctx = *reinterpret_cast<ParseContext<TCHAR>*>(context);
    SymbolContext<TCHAR> rc(ctx, *symbol, values, num_values);
    if (symbol->id < ctx.m_callbacks.size() &&
        ctx.m_callbacks[symbol->id] != nullptr)
      return (long)ctx.m_callbacks[symbol->id](rc);
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

        m_callbacks.resize(m_grammar.get_machine_num() + 1, nullptr);
    }
    void parse(const char *input_path, int worker_num)
    {
        int pid = get_current_pid();

        ParseContext<TCHAR> context{m_callbacks, nullptr};

        std::vector<BaseRunner *> runners;
        runners.push_back(new Stage1Runner{ input_path, &m_parser, 8 * 1024 * 1024, worker_num * 2 });
        for (int i = 0; i < worker_num; i++)
        {
            Stage2Runner *st2 = new Stage2Runner{input_path, 8 * 1024 * 1024, worker_num * 2, pid, static_cast<void *>(&context) };

            st2->register_listener(callback);

            runners.push_back(st2);
        }
        Stage3Runner *st3 = new Stage3Runner{ input_path, 8 * 1024 * 1024, worker_num * 2, pid, static_cast<void *>(&context) };

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
        for (auto p : runners)
        {
            delete p;
        }
    }
    void dry_parse(const char *input_path, int worker_num = 1)
    {
        Stage1Runner st{ input_path, &m_parser, 8 * 1024 * 1024, worker_num * 2, true };
        st.start();
        st.wait();
    }
    void attach(const Identifier& id, CppReductionCallback<TCHAR> callback)
    {
        int index = m_grammar.get_machine_id(id);

        m_callbacks[index] = callback;
    }
};
}
