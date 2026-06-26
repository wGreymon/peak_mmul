#include <algorithm>
#include <array>
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

void fill_inputs(
    std::array<std::vector<std::uint16_t>, peak_bf16::NumAInputs>& input_a,
    std::array<std::vector<std::uint16_t>, peak_bf16::NumBInputs>& input_b) {
    for (std::size_t ch = 0; ch < input_a.size(); ++ch) {
        for (std::size_t i = 0; i < input_a[ch].size(); ++i) {
            const float value =
                static_cast<float>(((i + ch) & 7u) + 1u) * 0.125f;
            input_a[ch][i] = float_to_bf16(value);
        }
    }
    for (std::size_t ch = 0; ch < input_b.size(); ++ch) {
        for (std::size_t i = 0; i < input_b[ch].size(); ++i) {
            const float value =
                static_cast<float>((((i * 3u) + ch) & 7u) + 1u) * 0.0625f;
            input_b[ch][i] = float_to_bf16(value);
        }
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

    std::array<std::vector<std::uint16_t>, peak_bf16::NumAInputs> input_a;
    std::array<std::vector<std::uint16_t>, peak_bf16::NumBInputs> input_b;
    std::vector<std::uint16_t> output(peak_bf16::CElements);
    for (auto& input : input_a) {
        input.resize(peak_bf16::AElements);
    }
    for (auto& input : input_b) {
        input.resize(peak_bf16::BElements);
    }
    fill_inputs(input_a, input_b);

    const std::uint16_t* input_a_ptrs[peak_bf16::NumAInputs];
    const std::uint16_t* input_b_ptrs[peak_bf16::NumBInputs];
    for (int i = 0; i < peak_bf16::NumAInputs; ++i) {
        input_a_ptrs[i] = input_a[i].data();
    }
    for (int i = 0; i < peak_bf16::NumBInputs; ++i) {
        input_b_ptrs[i] = input_b[i].data();
    }

    try {
        peak_bf16::run_harness_bf16(output.data(),
                                    input_a_ptrs,
                                    input_b_ptrs,
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
