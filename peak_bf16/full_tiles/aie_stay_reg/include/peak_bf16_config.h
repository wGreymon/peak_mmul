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

static constexpr int NumRows = 8;
static constexpr int NumCols = 38;
static constexpr int NumTiles = NumRows * NumCols;
static constexpr int NumOutputPlio = 16;
static constexpr int TilesPerOutput = NumTiles / NumOutputPlio;   // 每条IO输出 304 / 16 = 19 个tile的数据
static_assert(NumTiles % NumOutputPlio == 0);

static constexpr int NumGroupM = InputM / MmulM;
static constexpr int NumGroupK = InputK / MmulK;
static constexpr int NumGroupN = InputN / MmulN;

static constexpr int MmulMacs = MmulM * MmulK * MmulN;
static constexpr int OutputMmulCount = NumGroupM * NumGroupN;
static constexpr int MacsPerTileRun = OutputMmulCount * MmulMacs * NumGroupK;
static constexpr int FlopsPerTileRun = 2 * MacsPerTileRun;
static constexpr std::uint64_t MacsPerGraphRun =
    static_cast<std::uint64_t>(NumTiles) * MacsPerTileRun;
static constexpr std::uint64_t FlopsPerGraphRun =
    static_cast<std::uint64_t>(NumTiles) * FlopsPerTileRun;

static constexpr int MetadataWords = 7;
static constexpr int StreamHeaderWords = 1;
static constexpr int StreamPaddingWords = 2;
static constexpr int MetadataBytes = MetadataWords * sizeof(std::uint32_t);
static constexpr int OutputWordsPerPlio =
    StreamHeaderWords + TilesPerOutput * MetadataWords + StreamPaddingWords;
static constexpr std::size_t OutputBytesPerPlio =
    OutputWordsPerPlio * sizeof(std::uint32_t);
static_assert(OutputBytesPerPlio % 16 == 0);
static constexpr std::size_t TotalOutputBytes =
    NumOutputPlio * OutputBytesPerPlio;

static constexpr std::uint32_t DataFlag = 0x41494531;  // "AIE1"
static constexpr std::uint64_t AieClockHz = 1250000000ull;

static constexpr const char* GraphName = "gr_peak_bf16_full_tiles";

}  // namespace peak_bf16
