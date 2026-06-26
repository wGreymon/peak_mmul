#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <vector>

#include "peak_int8_config.h"
#include "peak_int8_harness.hpp"

namespace {

void fill_inputs(std::vector<std::int8_t>& input_a,
                 std::vector<std::int8_t>& input_b) {
    for (std::size_t i = 0; i < input_a.size(); ++i) {
        input_a[i] = static_cast<std::int8_t>((i & 7u) + 1u);
    }
    for (std::size_t i = 0; i < input_b.size(); ++i) {
        input_b[i] = static_cast<std::int8_t>(((i * 3u) & 7u) + 1u);
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::size_t graph_iterations = 1;
    if (argc > 1) {
        graph_iterations = static_cast<std::size_t>(std::strtoul(argv[1], nullptr, 10));
    }
    if (graph_iterations == 0) {
        std::cerr << "ERROR: graph_iterations must be positive.\n";
        return EXIT_FAILURE;
    }

    std::vector<std::int8_t> input_a(peak_int8::AElements);
    std::vector<std::int8_t> input_b(peak_int8::BElements);
    std::vector<std::uint8_t> output(peak_int8::CElements);
    fill_inputs(input_a, input_b);

    try {
        peak_int8::run_harness_int8(output.data(),
                                    input_a.data(),
                                    input_b.data(),
                                    graph_iterations);
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    std::uint64_t checksum = 0;
    int nonzero = 0;
    for (int i = 0; i < peak_int8::CResultElements; ++i) {
        const std::uint8_t value = output[i];
        checksum += value;
        if (value != 0) {
            ++nonzero;
        }
    }

    std::cout << "AIE output probe\n";
    std::cout << " - correctness check             : disabled\n";
    std::cout << " - result bytes scanned          : "
              << peak_int8::CResultElements << '\n';
    std::cout << " - metadata byte offset          : "
              << peak_int8::PerfByteOffset << '\n';
    std::cout << " - nonzero bytes                 : " << nonzero << '\n';
    std::cout << " - checksum_u64                  : " << checksum << '\n';
    std::cout << " - first bytes                   : "
              << "0x" << std::hex << static_cast<int>(output[0]) << " "
              << "0x" << static_cast<int>(output[1]) << " "
              << "0x" << static_cast<int>(output[2]) << " "
              << "0x" << static_cast<int>(output[3]) << std::dec << '\n';

    return EXIT_SUCCESS;
}
