#pragma once

#include <cstddef>
#include <cstdint>

namespace peak_bf16 {

static constexpr int MmulM = 8;
static constexpr int MmulK = 8;
static constexpr int MmulN = 4;

static constexpr int InputM = 64;
static constexpr int InputK = 96;
static constexpr int InputN = 64;

static constexpr int PackY = 1;
static constexpr int PackG = 4;
static constexpr int PackX = 1;
static constexpr int NumKernels = PackY * PackG * PackX;
static constexpr int NumAInputs = PackY * PackG;
static constexpr int NumBInputs = PackG * PackX;
static constexpr int NumCOutputs = PackY * PackX;

static constexpr int NumGroupM = InputM / MmulM;
static constexpr int NumGroupK = InputK / MmulK;
static constexpr int NumGroupN = InputN / MmulN;

static constexpr int MmulMacs = MmulM * MmulK * MmulN;
static constexpr int OutputMmulCount = NumGroupM * NumGroupN;
static constexpr int MacsPerKernelRun = OutputMmulCount * MmulMacs * NumGroupK;
static constexpr int MacsPerGraphRun = NumKernels * MacsPerKernelRun;
static constexpr int FlopsPerGraphRun = 2 * MacsPerGraphRun;

static constexpr int AElements = InputM * InputK;
static constexpr int BElements = InputK * InputN;
static constexpr int CElements = InputM * InputN;
static constexpr int PackedBf16PerWord = 2;
static constexpr int CMatrixWords = CElements / PackedBf16PerWord;
static constexpr int PerfOutputWords = 8;
static constexpr int PerfOutputElements = PerfOutputWords * PackedBf16PerWord;
static constexpr int PerfWordOffset = CMatrixWords - PerfOutputWords;
static constexpr int PerfElementOffset = PerfWordOffset * PackedBf16PerWord;
static constexpr int CResultElements = CElements - PerfOutputElements;

static constexpr std::size_t AInputBytes = AElements * sizeof(std::uint16_t);
static constexpr std::size_t BInputBytes = BElements * sizeof(std::uint16_t);
static constexpr std::size_t COutputBytes = CElements * sizeof(std::uint16_t);

static constexpr std::uint32_t PerfMagic = 0x41494531;  // "AIE1"
static constexpr std::uint32_t PerfVersion = 1;
static constexpr std::uint64_t AieClockHz = 1250000000ull;

static constexpr const char* GraphName = "gr_peak_bf16";

}  // namespace peak_bf16
