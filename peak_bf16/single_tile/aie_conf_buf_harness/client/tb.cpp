#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <vector>

#include "peak_bf16_config.h"
#include "peak_bf16_harness.hpp"

namespace {

std::uint16_t float_to_bf16(float value) {
    union {
        float f;
        std::uint32_t u;
    } bits{};
    bits.f = value;

    const std::uint32_t lsb = (bits.u >> 16) & 1u;
    const std::uint32_t rounding_bias = 0x7fffu + lsb;
    return static_cast<std::uint16_t>((bits.u + rounding_bias) >> 16);
}

void fill_inputs(std::vector<std::uint16_t>& input_a,
                 std::vector<std::uint16_t>& input_b) {
    for (std::size_t i = 0; i < input_a.size(); ++i) {
        const float value = static_cast<float>((i & 7u) + 1u) * 0.125f;
        input_a[i] = float_to_bf16(value);
    }
    for (std::size_t i = 0; i < input_b.size(); ++i) {
        const float value = static_cast<float>(((i * 3u) & 7u) + 1u) * 0.0625f;
        input_b[i] = float_to_bf16(value);
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

    std::vector<std::uint16_t> input_a(peak_bf16::AElements);
    std::vector<std::uint16_t> input_b(peak_bf16::BElements);
    std::vector<std::uint16_t> output(peak_bf16::CElements);
    fill_inputs(input_a, input_b);

    try {
        peak_bf16::run_harness_bf16(output.data(),
                                    input_a.data(),
                                    input_b.data(),
                                    graph_iterations);
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    std::uint64_t checksum = 0;
    int nonzero = 0;
    for (int i = 0; i < peak_bf16::CResultElements; ++i) {
        const std::uint16_t value = output[i];
        checksum += value;
        if (value != 0) {
            ++nonzero;
        }
    }

    std::cout << "AIE output check\n";
    std::cout << " - result elements checked       : "
              << peak_bf16::CResultElements << '\n';
    std::cout << " - metadata element offset       : "
              << peak_bf16::PerfElementOffset << '\n';
    std::cout << " - nonzero elements              : " << nonzero << '\n';
    std::cout << " - checksum_u64                  : " << checksum << '\n';
    std::cout << " - first elements                : "
              << "0x" << std::hex << output[0] << " "
              << "0x" << output[1] << " "
              << "0x" << output[2] << " "
              << "0x" << output[3] << std::dec << '\n';

    if (nonzero == 0 || checksum == 0) {
        std::cerr << "ERROR: output is all zero.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
