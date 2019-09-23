#include <vector>
#include <chrono>

#include "Context.hpp"
#include "pugixml.hpp"

using namespace Centaurus;

int placeholder;

void *parseDocument(const SymbolContext<char>& ctx)
{
  return ctx.value<int>(1);
}

static std::vector<std::vector<std::string>*> result;

void *parseDblpRoot(const SymbolContext<char>& ctx)
{
  for (int i = 0; i < ctx.count(); i++) {
    result.emplace_back(ctx.value<std::vector<std::string>>(i+1));
  }
  return &result;
}

void *parseArticle(const SymbolContext<char>& ctx)
{
  if (ctx.count() > 0) {
    auto ret = new std::vector<std::string>;
    pugi::xml_document doc;
    doc.load_buffer(ctx.start(), ctx.end() -  ctx.start());
    pugi::xpath_node_set authors = doc.select_nodes("/article/author/text()");
    for (auto& node : authors) {
      ret->emplace_back(node.node().value());
    }
    return ret;
  } else {
    return nullptr;
  }
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
    if (argc < 2) return 1;
    int worker_num = std::atoi(argv[1]);
    bool no_action = argc >= 3 && argv[2] == std::string("dry");
    bool size = argc >= 3 && argv[2] == std::string("size");
    bool debug = argc >= 3 && argv[2] == std::string("debug");

    const char *input_path = "../../datasets/dblp.xml";
    const char *grammar_path = "../../grammars/dblp.cgr";

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
      for (auto vec_ptr : result) {
        for (auto& author : *vec_ptr) {
          std::cout << author << std::endl;
        }
        std::cout << std::endl;
      }
    }
    return 0;
}
