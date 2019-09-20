#include <rapidjson/reader.h>
#include <rapidjson/filereadstream.h>

#include <iostream>
#include <chrono>
#include <cstdint>

#include "memory_input.hpp"

struct DryHandler
{
  bool Null() {return true;}
  bool Bool(bool b) {return true;}
  bool Int(int i) {return true;}
  bool Uint(unsigned u) {return true;}
  bool Int64(std::int64_t i) {return true;}
  bool Uint64(std::uint64_t u) {return true;}
  bool Double(double d) {return true;}
  bool RawNumber(const char *str, rapidjson::SizeType length, bool copy) {return true;}
  bool String(const char *str, rapidjson::SizeType length, bool copy) {return true;}
  bool StartObject() {return true;}
  bool Key(const char *str, rapidjson::SizeType length, bool copy) {return true;}
  bool EndObject(rapidjson::SizeType memberCount) {return true;}
  bool StartArray() {return true;}
  bool EndArray(rapidjson::SizeType memberCount) {return true;}
};

int main(int argc, const char *argv[])
{
  if (argc < 2) return 0;
#ifdef BUFFERED
  FILE *fp = std::fopen(argv[1], "r");
  char readbuffer[65536];
  rapidjson::FileReadStream is(fp, readbuffer, sizeof(readbuffer));
#else
  MemoryInput input(argv[1]);
  rapidjson::StringStream is(input.buffer());
#endif
  DryHandler handler;
  rapidjson::Reader reader;

  using namespace std::chrono;

  auto start = high_resolution_clock::now();;

  reader.Parse(is, handler);

  auto end = high_resolution_clock::now();;

  std::cout << duration_cast<milliseconds>(end - start).count() << std::endl;
}
