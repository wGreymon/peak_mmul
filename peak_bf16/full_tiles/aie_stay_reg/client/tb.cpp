#include <cstdlib>
#include <exception>
#include <iostream>

#include "peak_bf16_config.h"
#include "peak_bf16_harness.hpp"

int main(int argc, char** argv) {
    std::size_t graph_iterations = 1;
    if (argc > 1) {
        graph_iterations =
            static_cast<std::size_t>(std::strtoul(argv[1], nullptr, 10));
    }
    if (graph_iterations == 0) {
        std::cerr << "ERROR: graph_iterations must be positive.\n";
        return EXIT_FAILURE;
    }

    peak_bf16::OutputChannels output;

    try {
        peak_bf16::run_harness_bf16_full_tiles(output, graph_iterations);
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
