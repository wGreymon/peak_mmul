#include <adf.h>

#include <aie_api/aie.hpp>
#include <aie_api/aie_adf.hpp>
#include <cstdint>

#include "peak_config.h"

namespace {

using MMUL = aie::mmul<peak_mmul::MmulM,
                       peak_mmul::MmulK,
                       peak_mmul::MmulN,
                       bfloat16,
                       bfloat16>;

// The unroll factor is intentionally compile-time fixed. The runtime repeat
// count controls duration, while this block keeps the inner loop mostly MACs.
static constexpr int UnrollMacs = 64;

inline aie::vector<bfloat16, MMUL::size_A> make_a(int seed) {
    aie::vector<bfloat16, MMUL::size_A> v;
    for (int i = 0; i < MMUL::size_A; ++i) {
        const float x = static_cast<float>(((i + seed) & 7) + 1) * 0.125f;
        v.set(i, bfloat16(x));
    }
    return v;
}

inline aie::vector<bfloat16, MMUL::size_B> make_b(int seed) {
    aie::vector<bfloat16, MMUL::size_B> v;
    for (int i = 0; i < MMUL::size_B; ++i) {
        const float x = static_cast<float>(((i * 3 + seed) & 7) + 1) * 0.0625f;
        v.set(i, bfloat16(x));
    }
    return v;
}

}  // namespace

void peak_mmul_kernel(input_stream_uint32* input,
                      output_stream_uint32* output) {
    aie::set_rounding(aie::rounding_mode::symmetric_inf);
    aie::set_saturation(aie::saturation_mode::saturate);

    int repeats = static_cast<int>(readincr(input));
    if (repeats <= 0) {
        repeats = 1;
    }

    const auto a0 = make_a(0);
    const auto a1 = make_a(1);
    const auto b0 = make_b(0);
    const auto b1 = make_b(1);

    MMUL c0;
    MMUL c1;
    c0.mul(a0, b0);
    c1.mul(a1, b1);

    for (int r = 0; r < repeats; ++r)
        chess_prepare_for_pipelining {
            for (int u = 0; u < UnrollMacs; ++u) {
                if ((u & 1) == 0) {
                    c0.mac(a0, b0);
                } else {
                    c1.mac(a1, b1);
                }
            }
        }

    const auto out0 = c0.to_vector<float>();
    const auto out1 = c1.to_vector<float>();
    float checksum = 0.0f;

    for (int i = 0; i < MMUL::size_C; ++i) {
        checksum += out0.get(i) + out1.get(i);
    }

    std::uint32_t checksum_bits = *reinterpret_cast<std::uint32_t*>(&checksum);

    writeincr(output, static_cast<std::uint32_t>(repeats));
    writeincr(output, static_cast<std::uint32_t>(UnrollMacs));
    writeincr(output, static_cast<std::uint32_t>(peak_mmul::MacsPerMmul));
    writeincr(output, checksum_bits);
    writeincr(output, static_cast<std::uint32_t>(0x5045414bu)); // "PEAK"
    writeincr(output, static_cast<std::uint32_t>(0));
    writeincr(output, static_cast<std::uint32_t>(0));
    writeincr(output, static_cast<std::uint32_t>(0));
}
