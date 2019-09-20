#include <rapidjson/document.h>

#include <iostream>
#include <chrono>

#include "memory_input.hpp"

int main(int argc, const char *argv[])
{
  if (argc < 2) return 0;
  MemoryInput input(argv[1]);

  rapidjson::Document document;

  using namespace std::chrono;

  auto start = high_resolution_clock::now();;

  document.Parse(input.buffer());

  auto end = high_resolution_clock::now();;

  std::cout << duration_cast<milliseconds>(end - start).count() << std::endl;
}
