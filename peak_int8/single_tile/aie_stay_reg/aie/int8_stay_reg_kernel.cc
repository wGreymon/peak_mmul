#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/aie_adf.hpp>
#include <cstdint>

#include "peak_int8_config.h"

namespace {

using MMUL = aie::mmul<peak_int8::MmulM,
                       peak_int8::MmulK,
                       peak_int8::MmulN,
                       int8_t,
                       int8_t>;

static_assert(MMUL::size_A == peak_int8::MmulM * peak_int8::MmulK);
static_assert(MMUL::size_B == peak_int8::MmulK * peak_int8::MmulN);
static_assert(MMUL::size_C == peak_int8::MmulM * peak_int8::MmulN);

static constexpr int MmulOpsPerOuterLoop = 2;
static constexpr int ComputeOuterLoops =
    peak_int8::OutputMmulCount * peak_int8::NumGroupK /
    MmulOpsPerOuterLoop;

static_assert(peak_int8::OutputMmulCount * peak_int8::NumGroupK %
                  MmulOpsPerOuterLoop == 0);

inline void write_perf_metadata(uint8_t* __restrict pC,
                                std::uint64_t start_cycles,
                                std::uint64_t end_cycles) {
    const std::uint64_t delta_cycles = end_cycles - start_cycles;
    std::uint32_t* __restrict perf =
        reinterpret_cast<std::uint32_t*>(pC + peak_int8::PerfByteOffset);

    perf[0] = peak_int8::PerfMagic;
    perf[1] = peak_int8::PerfVersion;
    perf[2] = static_cast<std::uint32_t>(start_cycles);
    perf[3] = static_cast<std::uint32_t>(start_cycles >> 32);
    perf[4] = static_cast<std::uint32_t>(end_cycles);
    perf[5] = static_cast<std::uint32_t>(end_cycles >> 32);
    perf[6] = static_cast<std::uint32_t>(delta_cycles);
    perf[7] = static_cast<std::uint32_t>(delta_cycles >> 32);
}

}  // namespace

void int8_stay_reg_kernel(adf::input_buffer<int8_t>& __restrict matA,
                          adf::input_buffer<int8_t>& __restrict matB,
                          adf::output_buffer<uint8_t>& __restrict matC) {
    aie::set_rounding(aie::rounding_mode::symmetric_inf);
    aie::set_saturation(aie::saturation_mode::saturate);

    const int8_t* __restrict pA = matA.data();
    const int8_t* __restrict pB = matB.data();
    uint8_t* __restrict pC = matC.data();

    aie::vector<int8_t, MMUL::size_A> A0 = aie::load_v<MMUL::size_A>(pA);
    aie::vector<int8_t, MMUL::size_A> A1 =
        aie::load_v<MMUL::size_A>(pA + MMUL::size_A);
    aie::vector<int8_t, MMUL::size_B> B0 = aie::load_v<MMUL::size_B>(pB);
    aie::vector<int8_t, MMUL::size_B> B1 =
        aie::load_v<MMUL::size_B>(pB + MMUL::size_B);

    MMUL C0;
    MMUL C1;
    C0.mul(A0, B0);
    C1.mul(A1, B1);

    aie::tile tile = aie::tile::current();
    const std::uint64_t start_cycles = tile.cycles();

    for (int i = 0; i < ComputeOuterLoops; ++i)
        chess_prepare_for_pipelining {
            C0.mac(A0, B0);
            C1.mac(A1, B1);
        }

    const std::uint64_t end_cycles = tile.cycles();

    int8_t* __restrict out = reinterpret_cast<int8_t*>(pC);
    aie::store_v(out, C0.template to_vector<int8_t>());
    aie::store_v(out + MMUL::size_C, C1.template to_vector<int8_t>());
    write_perf_metadata(pC, start_cycles, end_cycles);
}
