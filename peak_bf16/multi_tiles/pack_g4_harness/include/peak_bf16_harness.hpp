#pragma once

#include <cstddef>
#include <cstdint>

#include "peak_bf16_config.h"

namespace peak_bf16 {

void run_harness_bf16(std::uint16_t* output,
                      const std::uint16_t* const input_a[NumAInputs],
                      const std::uint16_t* const input_b[NumBInputs],
                      std::size_t graph_iterations);

}  // namespace peak_bf16
