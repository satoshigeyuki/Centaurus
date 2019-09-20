#include "Context.hpp"
#include "rapidjson/document.h"

#include <vector>
#include <chrono>
#include <iostream>

using namespace Centaurus;

int placeholder;

void *parseRootObject(const SymbolContext<char>& ctx)
{
  return ctx.value<std::vector<rapidjson::Document*>>(1);
}

void *parseFeatureList(const SymbolContext<char>& ctx)
{
  auto ret = new std::vector<rapidjson::Document*>;
  auto& vec = *ret;
  for (int i = 1; i <= ctx.count(); i++) {
    vec.emplace_back(ctx.value<rapidjson::Document>(i));
  }
  return ret;
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
  if (argc < 1) return 1;
  int worker_num = atoi(argv[1]);

  const char *input_path = "../../datasets/citylots.json";
  const char *grammar_path = "../../grammars/citylots2.cgr";

  Context<char> context{grammar_path};

  context.attach(L"RootObject", parseRootObject);
  context.attach(L"FeatureList", parseFeatureList);
  context.attach(L"FeatureDict", parseFeatureDict);
  context.attach(L"PropertyDict", parsePropertyDict);
  context.attach(L"TargetPropertyEntry", parseTargetPropertyEntry);

  using namespace std::chrono;

  auto start = high_resolution_clock::now();;

  context.parse(input_path, worker_num);

  auto end = high_resolution_clock::now();;

  std::cout << worker_num << " " << duration_cast<milliseconds>(end - start).count() << std::endl;
}
