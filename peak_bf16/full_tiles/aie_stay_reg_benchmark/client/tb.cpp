#include <exception>
#include <iostream>

#include "peak_bf16_config.h"
#include "peak_bf16_harness.hpp"

int main() {
    peak_bf16::OutputChannels output;

    try {
        peak_bf16::run_harness_bf16_full_tiles(output);
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
