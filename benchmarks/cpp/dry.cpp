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

    const int worker_num = 1;

    using namespace std::chrono;

    auto start = high_resolution_clock::now();;

    Centaurus::Stage1Runner runner{input_path, &parser, 8 * 1024 * 1024,  worker_num * 2, true, result_captured};
    runner.start();
    runner.wait();

    auto end = high_resolution_clock::now();;

    if (result_captured) {
      for (auto& chunk : runner.result_chunks()) {
        for (auto& m : chunk) {
          if (m.get_machine_id() == 0 && m.get_offset() == 0) break;
          std::cout << (m.is_start_marker() ? "S " : "E ") << m.get_machine_id() << "\t" << m.get_offset() << std::endl;
        }
      }
    } else {
      std::cout << duration_cast<milliseconds>(end - start).count() << std::endl;
    }

    return 0;
}
