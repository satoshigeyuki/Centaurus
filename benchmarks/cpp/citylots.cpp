#include "Context.hpp"
#include "rapidjson/document.h"

#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/writer.h"

#include <vector>
#include <chrono>
#include <iostream>

using namespace Centaurus;

int placeholder;

void *parseRootObject(const SymbolContext<char>& ctx)
{
  return ctx.value<std::vector<rapidjson::Document*>>(1);
}

static std::vector<rapidjson::Document*> result;

void *parseFeatureList(const SymbolContext<char>& ctx)
{
  for (int i = 1; i <= ctx.count(); i++) {
    result.emplace_back(ctx.value<rapidjson::Document>(i));
  }
  return &result;
}

void *parseFeatureDict(const SymbolContext<char>& ctx)
{
  if (ctx.count() > 0) {
    auto ret = new rapidjson::Document;
    ret->Parse(ctx.read().c_str());
    return ret;
  }
  return nullptr;
}

void *parsePropertyDict(const SymbolContext<char>& ctx)
{
  if (ctx.count() > 0) {
    return &placeholder;
  }
  return nullptr;
}

void *parseTargetPropertyEntry(const SymbolContext<char>& ctx)
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

  const char *input_path = "../../datasets/citylots.json";
  const char *grammar_path = "../../grammars/citylots.cgr";

  Context<char> context{grammar_path};

  if (!no_action) {
    context.attach(L"RootObject", parseRootObject);
    context.attach(L"FeatureList", parseFeatureList);
    context.attach(L"FeatureDict", parseFeatureDict);
    context.attach(L"PropertyDict", parsePropertyDict);
    context.attach(L"TargetPropertyEntry", parseTargetPropertyEntry);
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
    for (auto doc_ptr : result) {
      rapidjson::OStreamWrapper osw(std::cout);
      rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
      doc_ptr->Accept(writer);
    }
    std::cout << std::endl;
  }
}
