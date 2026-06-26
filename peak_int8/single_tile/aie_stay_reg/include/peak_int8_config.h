#pragma once

#include <cstddef>
#include <cstdint>

namespace peak_int8 {

static constexpr int MmulM = 4;
static constexpr int MmulK = 8;
static constexpr int MmulN = 8;

static constexpr int InputM = 64 * 2;
static constexpr int InputK = 96;
static constexpr int InputN = 64;

static constexpr int NumGroupM = InputM / MmulM;
static constexpr int NumGroupK = InputK / MmulK;
static constexpr int NumGroupN = InputN / MmulN;

static constexpr int MmulMacs = MmulM * MmulK * MmulN;
static constexpr int OutputMmulCount = NumGroupM * NumGroupN;
static constexpr int MacsPerGraphRun = OutputMmulCount * MmulMacs * NumGroupK;
static constexpr int OpsPerGraphRun = 2 * MacsPerGraphRun;

static constexpr int AElements = InputM * InputK;
static constexpr int BElements = InputK * InputN;
static constexpr int CElements = InputM * InputN;
static constexpr int PerfOutputBytes = 32;
static constexpr int PerfByteOffset = CElements - PerfOutputBytes;
static constexpr int CResultElements = CElements - PerfOutputBytes;

static constexpr std::size_t AInputBytes = AElements * sizeof(std::int8_t);
static constexpr std::size_t BInputBytes = BElements * sizeof(std::int8_t);
static constexpr std::size_t COutputBytes = CElements * sizeof(std::uint8_t);

static constexpr std::uint32_t PerfMagic = 0x41494531;  // "AIE1"
static constexpr std::uint32_t PerfVersion = 1;
static constexpr std::uint64_t AieClockHz = 1250000000ull;

static constexpr const char* GraphName = "gr_peak_int8";
static constexpr const char* APlioInName = "PLIO_01_TO_AIE";
static constexpr const char* BPlioInName = "PLIO_02_TO_AIE";
static constexpr const char* PlioOutName = "PLIO_01_FROM_AIE";

}  // namespace peak_int8
