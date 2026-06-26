#pragma once

#include <cstddef>
#include <cstdint>

namespace peak_int8 {

void run_harness_int8(std::uint8_t* output,
                      const std::int8_t* input_a,
                      const std::int8_t* input_b,
                      std::size_t graph_iterations);

}  // namespace peak_int8
