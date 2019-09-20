#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>

#include <cstdint>
#include <chrono>
#include <iostream>

int main(int argc, const char *argv[])
{
  if (argc < 2) return 0;
  const char *input_path = argv[1];

  auto *fp = std::fopen(input_path, "r");
  char readbuffer[65536];
  rapidjson::FileReadStream is(fp, readbuffer, sizeof(readbuffer));
  rapidjson::Document document;

  using namespace std::chrono;

  auto start = high_resolution_clock::now();;

  document.ParseStream(is);

  auto end = high_resolution_clock::now();;

  std::cout << duration_cast<milliseconds>(end - start).count() << std::endl;

  std::fclose(fp);
}
