#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "peak_bf16_config.h"

namespace peak_bf16 {

using OutputChannels =
    std::array<std::vector<std::uint32_t>, NumOutputPlio>;

void run_harness_bf16_full_tiles(OutputChannels& output,
                                 std::size_t graph_iterations);

}  // namespace peak_bf16
