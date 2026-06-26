#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/aie_adf.hpp>
#include <cstdint>

#include "peak_int8_config.h"

namespace {

using MMUL = aie::mmul<peak_int8::M,
                       peak_int8::K,
                       peak_int8::N,
                       int8_t,
                       int8_t>;

inline aie::vector<int8_t, MMUL::size_A> make_a(int seed) {
    aie::vector<int8_t, MMUL::size_A> v;
    for (int i = 0; i < MMUL::size_A; ++i) {
        v.set(i, static_cast<int8_t>(((i + seed) & 7) + 1));
    }
    return v;
}

inline aie::vector<int8_t, MMUL::size_B> make_b(int seed) {
    aie::vector<int8_t, MMUL::size_B> v;
    for (int i = 0; i < MMUL::size_B; ++i) {
        v.set(i, static_cast<int8_t>(((i * 3 + seed) & 7) + 1));
    }
    return v;
}

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────
// peak_int8_kernel
//
// Strategy: same as BF16 — pre-load A/B tiles, hammer mac() in a tight loop.
// int8 on AIE-ML has 2× the MAC density of BF16 because each MAC unit
// processes two 8-bit operands instead of two 16-bit.
//
// Two parallel mmul units × 256 MACs each = 512 MACs/iteration.
// At 312.5 MHz: 312.5M × 512 = 160 GOPS per tile.
// With 304 tiles: theoretical peak ≈ 48.6 TOPS.
// GAMA achieves 165 TOPS (85%) with large matrix tiling (better MAC utilization).
// ─────────────────────────────────────────────────────────────────────────────
void peak_int8_kernel(input_stream_uint32* input,
                     output_stream_uint32* output) {
    aie::set_rounding(aie::rounding_mode::symmetric_inf);
    aie::set_saturation(aie::saturation_mode::saturate);

    int repeats = static_cast<int>(readincr(input));
    if (repeats <= 0) {
        repeats = 1;
    }

    auto a0 = make_a(0);
    auto a1 = make_a(1);
    auto b0 = make_b(0);
    auto b1 = make_b(1);

    MMUL c0;
    MMUL c1;
    c0.mul(a0, b0);
    c1.mul(a1, b1);

    for (int r = 0; r < repeats; ++r)
        chess_prepare_for_pipelining {
            c0.mac(a0, b0);
            c1.mac(a1, b1);
        }

    const auto out0 = c0.to_vector<int32_t>();
    const auto out1 = c1.to_vector<int32_t>();
    std::int32_t checksum = 0;
    for (int i = 0; i < MMUL::size_C; ++i) {
        checksum += out0.get(i) + out1.get(i);
    }

    writeincr(output, static_cast<std::uint32_t>(repeats));
    writeincr(output, static_cast<std::uint32_t>(2));                    // 2 mmul units
    writeincr(output, static_cast<std::uint32_t>(peak_int8::MacsPerMmul)); // MACs per mmul
    writeincr(output, static_cast<std::uint32_t>(static_cast<std::uint32_t>(checksum)));
    writeincr(output, static_cast<std::uint32_t>(0x494E5438u));          // "INT8"
    writeincr(output, static_cast<std::uint32_t>(0));
    writeincr(output, static_cast<std::uint32_t>(0));
    writeincr(output, static_cast<std::uint32_t>(0));
}
