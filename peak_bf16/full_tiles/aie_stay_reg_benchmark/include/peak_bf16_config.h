#pragma once

#include <cstddef>
#include <cstdint>

namespace peak_bf16 {

static constexpr int MmulM = 8;
static constexpr int MmulK = 8;
static constexpr int MmulN = 4;

static constexpr int TinyM = 64;
static constexpr int TinyK = 96;
static constexpr int TinyN = 64;
static constexpr int MediumM = 512;
static constexpr int MediumK = 768;
static constexpr int MediumN = 512;
static constexpr int HugeM = 4096;
static constexpr int HugeK = 8192;
static constexpr int HugeN = 4096;

static constexpr int NumRows = 8;
static constexpr int NumCols = 38;
static constexpr int NumTiles = NumRows * NumCols;
static constexpr int NumInputPlio = 16;
static constexpr int NumOutputPlio = 16;
static constexpr int TilesPerOutput = NumTiles / NumOutputPlio;   // 每条IO输出 304 / 16 = 19 个tile的数据
static_assert(NumTiles % NumOutputPlio == 0);
static_assert(NumInputPlio == NumOutputPlio);
static_assert(TinyM % MmulM == 0 && TinyK % MmulK == 0 &&
              TinyN % MmulN == 0);
static_assert(MediumM % MmulM == 0 && MediumK % MmulK == 0 &&
              MediumN % MmulN == 0);
static_assert(HugeM % MmulM == 0 && HugeK % MmulK == 0 &&
              HugeN % MmulN == 0);

static constexpr std::uint64_t MmulMacs =
    static_cast<std::uint64_t>(MmulM) * MmulK * MmulN;

constexpr std::uint32_t loop_count_for_shape(int m, int k, int n) {
    return static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(m / MmulM) * (k / MmulK) *
         (n / MmulN)) /
        4);
}

static constexpr std::uint32_t TinyLoopCount =
    loop_count_for_shape(TinyM, TinyK, TinyN);
static constexpr std::uint32_t MediumLoopCount =
    loop_count_for_shape(MediumM, MediumK, MediumN);
static constexpr std::uint32_t HugeLoopCount =
    loop_count_for_shape(HugeM, HugeK, HugeN);

static constexpr int MetadataWords = 7;
static constexpr int StreamHeaderWords = 2;
static constexpr int StreamPaddingWords = 1;
static constexpr int MetadataBytes = MetadataWords * sizeof(std::uint32_t);
static constexpr int ControlWordsPerPlio = 4;
static constexpr std::size_t ControlBytesPerPlio =
    ControlWordsPerPlio * sizeof(std::uint32_t);
static_assert(ControlBytesPerPlio % 16 == 0);
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
