#include <pugixml.hpp>

#include <iostream>
#include <chrono>

int main(int argc, const char *argv[])
{
    if (argc < 2) return 0;
    const char *input_path = argv[1];

    pugi::xml_document doc;

    using namespace std::chrono;

    auto start = high_resolution_clock::now();;

    pugi::xml_parse_result result = doc.load_file(input_path);

    auto end = high_resolution_clock::now();;

    std::cout << duration_cast<milliseconds>(end - start).count() << std::endl;
}
