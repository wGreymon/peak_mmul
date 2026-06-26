#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/aie_adf.hpp>
#include <cstdint>

#include "peak_bf16_config.h"

namespace {

using MMUL = aie::mmul<peak_bf16::MmulM,
                       peak_bf16::MmulK,
                       peak_bf16::MmulN,
                       bfloat16,
                       bfloat16,
                       accauto>;

static_assert(MMUL::size_A == peak_bf16::MmulM * peak_bf16::MmulK);
static_assert(MMUL::size_B == peak_bf16::MmulK * peak_bf16::MmulN);
static_assert(MMUL::size_C == peak_bf16::MmulM * peak_bf16::MmulN);

static constexpr int MmulOpsPerOuterLoop = 4;
static constexpr int ComputeOuterLoops =
    peak_bf16::OutputMmulCount * peak_bf16::NumGroupK /
    MmulOpsPerOuterLoop;

static_assert(peak_bf16::OutputMmulCount * peak_bf16::NumGroupK %
                  MmulOpsPerOuterLoop ==
              0);

inline void write_perf_metadata(bfloat16* __restrict pC,
                                std::uint64_t start_cycles,
                                std::uint64_t end_cycles) {
    const std::uint64_t delta_cycles = end_cycles - start_cycles;
    std::uint32_t* __restrict perf =
        reinterpret_cast<std::uint32_t*>(pC + peak_bf16::PerfElementOffset);

    perf[0] = peak_bf16::PerfMagic;
    perf[1] = peak_bf16::PerfVersion;
    perf[2] = static_cast<std::uint32_t>(start_cycles);
    perf[3] = static_cast<std::uint32_t>(start_cycles >> 32);
    perf[4] = static_cast<std::uint32_t>(end_cycles);
    perf[5] = static_cast<std::uint32_t>(end_cycles >> 32);
    perf[6] = static_cast<std::uint32_t>(delta_cycles);
    perf[7] = static_cast<std::uint32_t>(delta_cycles >> 32);
}

}  // namespace

void bf16_local_buffer_kernel(adf::input_buffer<bfloat16>& __restrict matA,
                              adf::input_buffer<bfloat16>& __restrict matB,
                              adf::output_buffer<bfloat16>& __restrict matC) {
    aie::set_rounding(aie::rounding_mode::symmetric_inf);
    aie::set_saturation(aie::saturation_mode::saturate);

    const bfloat16* __restrict pA = matA.data();
    const bfloat16* __restrict pB = matB.data();
    bfloat16* __restrict pC = matC.data();

    aie::vector<bfloat16, MMUL::size_A> A0 = aie::load_v<MMUL::size_A>(pA);
    aie::vector<bfloat16, MMUL::size_A> A1 =
        aie::load_v<MMUL::size_A>(pA + MMUL::size_A);
    aie::vector<bfloat16, MMUL::size_B> B0 = aie::load_v<MMUL::size_B>(pB);
    aie::vector<bfloat16, MMUL::size_B> B1 =
        aie::load_v<MMUL::size_B>(pB + MMUL::size_B);

    MMUL C00;
    MMUL C01;
    MMUL C10;
    MMUL C11;

    C00.mul(A0, B0);
    C01.mul(A0, B1);
    C10.mul(A1, B0);
    C11.mul(A1, B1);

    aie::tile tile = aie::tile::current();
    const std::uint64_t start_cycles = tile.cycles();

    for (int i = 0; i < ComputeOuterLoops; ++i)
        chess_prepare_for_pipelining {
            C00.mac(A0, B0);
            C01.mac(A0, B1);
            C10.mac(A1, B0);
            C11.mac(A1, B1);
        }

    const std::uint64_t end_cycles = tile.cycles();

    aie::store_v(pC, C00.template to_vector<bfloat16>());
    aie::store_v(pC + MMUL::size_C, C01.template to_vector<bfloat16>());
    aie::store_v(pC + 2 * MMUL::size_C, C10.template to_vector<bfloat16>());
    aie::store_v(pC + 3 * MMUL::size_C, C11.template to_vector<bfloat16>());
    write_perf_metadata(pC, start_cycles, end_cycles);
}
