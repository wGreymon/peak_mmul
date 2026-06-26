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

static constexpr int NumGroupM = InputM / MmulM;
static constexpr int NumGroupK = InputK / MmulK;
static constexpr int NumGroupN = InputN / MmulN;

static constexpr int MmulMacs = MmulM * MmulK * MmulN;
static constexpr int OutputMmulCount = NumGroupM * NumGroupN;
static constexpr int MacsPerRepeat = OutputMmulCount * MmulMacs * NumGroupK;

static constexpr std::uint32_t DefaultRepeats = 100;

static constexpr int AElements = InputM * InputK;
static constexpr int BElements = InputK * InputN;
static constexpr int CElements = InputM * InputN;
static constexpr int PackedBf16PerWord = 2;
static constexpr int AInputWords = AElements / PackedBf16PerWord;
static constexpr int BInputWords = BElements / PackedBf16PerWord;
static constexpr int CMatrixWords = CElements / PackedBf16PerWord;
static constexpr int PerfOutputWords = 8;
static constexpr int PerfWordOffset = CMatrixWords - PerfOutputWords;
static constexpr int CResultWords = CMatrixWords - PerfOutputWords;
static constexpr int COutputWords = CMatrixWords;
static constexpr int COutputElements = CElements;

static constexpr std::size_t AInputBytes = AInputWords * sizeof(std::uint32_t);
static constexpr std::size_t BInputBytes = BInputWords * sizeof(std::uint32_t);
static constexpr std::size_t COutputBytes = COutputWords * sizeof(std::uint32_t);

static constexpr std::uint32_t PerfMagic = 0x41494531;  // "AIE1"
static constexpr std::uint32_t PerfVersion = 1;
static constexpr std::uint64_t AieClockHz = 1250000000ull;

static constexpr const char* GraphName = "peak_bf16_single_tile_graph";
static constexpr const char* APlioInName = "AIn0";
static constexpr const char* BPlioInName = "BIn0";
static constexpr const char* PlioOutName = "DataOut0";

}  // namespace peak_bf16
