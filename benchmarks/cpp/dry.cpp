#include <chrono>

#include "Context.hpp"

int main(int argc, const char *argv[])
{
    if (argc < 3) return 0;

    const char *grammar_path = argv[1];
    const char *input_path = argv[2];

    Centaurus::Context<char> context{grammar_path};

    using namespace std::chrono;

    auto start = high_resolution_clock::now();;

    context.dry_parse(input_path);

    auto end = high_resolution_clock::now();;

    std::cout << duration_cast<milliseconds>(end - start).count() << std::endl;

    return 0;
}
