#pragma once

#include <cstddef>
#include <cstdint>

namespace peak_bf16 {

void run_harness_bf16(std::uint16_t* output,
                      const std::uint16_t* input_a,
                      const std::uint16_t* input_b,
                      std::size_t graph_iterations);

}  // namespace peak_bf16
