#include <chrono>

#include "CodeGenEM64T.hpp"
#include "Stage1Runner.hpp"

int main(int argc, const char *argv[])
{
    if (argc < 3) return 0;

    const char *grammar_path = argv[1];
    const char *input_path = argv[2];
    const bool result_captured = argc > 3 && argv[3] == std::string("debug");

    Centaurus::Grammar<char> grammar;
    Centaurus::ParserEM64T<char> parser;
    grammar.parse(grammar_path);
    parser.init(grammar);

    const int bank_size = 8 * 1024 * 1024;
    const int worker_num = 1;
    const int num_banks = worker_num * 2;

    using namespace std::chrono;

    auto start = high_resolution_clock::now();;

    Centaurus::Stage1Runner runner{input_path, &parser, bank_size, num_banks, true, result_captured};
    runner.start();
    runner.wait();

    auto end = high_resolution_clock::now();;

    if (result_captured) {
      for (auto& chunk : runner.result_chunks()) {
        for (auto& m : chunk) {
          std::cout << m.get_machine_id() << "\t" << m.get_offset() << std::endl;
        }
      }
    } else {
      std::cout << duration_cast<milliseconds>(end - start).count() << std::endl;
    }

    return 0;
}
