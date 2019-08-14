#include <map>
#include <chrono>

#include "Context.hpp"

using namespace Centaurus;

int placeholder;

void *parseDocument(const SymbolContext<char>& ctx)
{
  return ctx.value<int>(1);
}

static std::vector<std::string*> result;

void *parseDblpRoot(const SymbolContext<char>& ctx)
{
  for (int i = 0; i < ctx.count(); i++) {
    result.emplace_back(ctx.value<std::string>(i+1));
  }
  return &result;
}

void *parseArticle(const SymbolContext<char>& ctx)
{
  return (ctx.count() > 0) ? new std::string(ctx.start(), ctx.len()) : nullptr;
}

void *parseYearInfo(const SymbolContext<char>& ctx)
{
  return (ctx.count() > 0) ? &placeholder : nullptr;
}

void *parseTargetYearInfo(const SymbolContext<char>& ctx)
{
  return &placeholder;
}

int main(int argc, const char *argv[])
{
    if (argc < 1) return 1;
    int worker_num = std::atoi(argv[1]);
    bool no_action = argc >= 3 && argv[2] == std::string("dry");
    bool size = argc >= 3 && argv[2] == std::string("size");
    bool debug = argc >= 3 && argv[2] == std::string("debug");

    const char *input_path = "datasets/dblp.xml";
    const char *grammar_path = "grammar/dblp.cgr";

    Context<char> context{grammar_path};

    if (!no_action) {
      context.attach(L"Document", parseDocument);
      context.attach(L"DblpRoot", parseDblpRoot);
      context.attach(L"Article", parseArticle);
      context.attach(L"YearInfo", parseYearInfo);
      context.attach(L"TargetYearInfo", parseTargetYearInfo);
    }

    using namespace std::chrono;

    auto start = high_resolution_clock::now();;

    context.parse(input_path, worker_num);

    auto end = high_resolution_clock::now();;

    std::cout << worker_num << " " << duration_cast<milliseconds>(end - start).count() << std::endl;

    if (size) {
      std::cout << result.size() << std::endl;
    }
    if (debug) {
      for (auto p : result) {
        std::cout << *p << std::endl;
      }
    }
    return 0;
}
