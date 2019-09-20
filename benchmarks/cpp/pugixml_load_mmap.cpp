#include <pugixml.hpp>

#include <iostream>
#include <chrono>

#include "memory_input.hpp"

int main(int argc, const char *argv[])
{
    if (argc < 2) return 0;
    MemoryInput input(argv[1]);

    pugi::xml_document doc;

    using namespace std::chrono;

    auto start = high_resolution_clock::now();

    pugi::xml_parse_result result = doc.load_buffer(input.buffer(), input.size());

    auto end = high_resolution_clock::now();;

    std::cout << duration_cast<milliseconds>(end - start).count() << std::endl;
}
