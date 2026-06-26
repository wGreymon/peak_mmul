#pragma once

#include <cstddef>
#include <cstdint>

namespace peak_mmul {

static constexpr int MmulM = 4;
static constexpr int MmulK = 8;
static constexpr int MmulN = 8;
static constexpr int MacsPerMmul = MmulM * MmulK * MmulN;
static constexpr int FlopsPerMmul = 2 * MacsPerMmul;

// The host sends one 32-bit word. It is the outer repeat count used by the AIE
// kernel. Keep the stream tiny so this benchmark measures compute, not IO.
static constexpr int InputWords = 1;
static constexpr int OutputWords = 8;

static constexpr std::size_t InputBytes = InputWords * sizeof(std::uint32_t);
static constexpr std::size_t OutputBytes = OutputWords * sizeof(std::uint32_t);

static constexpr const char* GraphName = "peak_mmul_graph";
static constexpr const char* PlioInName = "DataIn0";
static constexpr const char* PlioOutName = "DataOut0";

}  // namespace peak_mmul

