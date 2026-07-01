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

static constexpr int MmulOpsPerOuterLoop = 4;

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

inline std::uint32_t checksum_accum(const MMUL& c0,
                                    const MMUL& c1,
                                    const MMUL& c2,
                                    const MMUL& c3) {
    const auto v0 = c0.template to_vector<bfloat16>();
    const auto v1 = c1.template to_vector<bfloat16>();
    const auto v2 = c2.template to_vector<bfloat16>();
    const auto v3 = c3.template to_vector<bfloat16>();
    std::uint32_t checksum = 0;
    for (int i = 0; i < MMUL::size_C; ++i) {
        checksum += static_cast<std::uint32_t>(v0.get(i));
        checksum += static_cast<std::uint32_t>(v1.get(i));
        checksum += static_cast<std::uint32_t>(v2.get(i));
        checksum += static_cast<std::uint32_t>(v3.get(i));
    }
    return checksum;
}

inline void run_compute(std::uint64_t& start_cycles,
                        std::uint64_t& end_cycles,
                        std::uint32_t& checksum,
                        std::uint32_t loop_count) {
    aie::set_rounding(aie::rounding_mode::symmetric_inf);
    aie::set_saturation(aie::saturation_mode::saturate);

    const auto A0 = make_a(0);
    const auto A1 = make_a(1);
    const auto B0 = make_b(0);
    const auto B1 = make_b(1);

    MMUL C00;
    MMUL C01;
    MMUL C10;
    MMUL C11;

    C00.mul(A0, B0);
    C01.mul(A0, B1);
    C10.mul(A1, B0);
    C11.mul(A1, B1);

    aie::tile tile = aie::tile::current();
    start_cycles = tile.cycles();

    for (std::uint32_t i = 0; i < loop_count; ++i)
        chess_prepare_for_pipelining {
            C00.mac(A0, B0);
            C01.mac(A0, B1);
            C10.mac(A1, B0);
            C11.mac(A1, B1);
        }

    end_cycles = tile.cycles();
    checksum = checksum_accum(C00, C01, C10, C11);
}

inline void write_record(output_stream_uint32* out,
                         std::uint32_t local_id,
                         std::uint64_t start_cycles,
                         std::uint64_t end_cycles,
                         std::uint32_t checksum) {
    writeincr(out, peak_bf16::DataFlag);
    writeincr(out, local_id);
    writeincr(out, checksum);
    writeincr(out, static_cast<std::uint32_t>(start_cycles));
    writeincr(out, static_cast<std::uint32_t>(start_cycles >> 32));
    writeincr(out, static_cast<std::uint32_t>(end_cycles));
    writeincr(out, static_cast<std::uint32_t>(end_cycles >> 32));
}

inline std::uint32_t read_control(input_stream_uint32* control) {
    const std::uint32_t loop_count = readincr(control);
    for (int i = 1; i < peak_bf16::ControlWordsPerPlio; ++i) {
        (void)readincr(control);
    }
    return loop_count;
}

inline void forward_records(input_stream_uint32* in,
                            output_stream_uint32* out,
                            int record_count) {
    for (int r = 0; r < record_count; ++r)
        chess_prepare_for_pipelining {
            for (int w = 0; w < peak_bf16::MetadataWords; ++w) {
                writeincr(out, readincr(in));
            }
        }
}

}  // namespace

void bf16_full_tile_first(input_stream_uint32* control,
                          output_stream_uint32* out) {
    std::uint64_t start_cycles = 0;
    std::uint64_t end_cycles = 0;
    std::uint32_t checksum = 0;
    const std::uint32_t loop_count = read_control(control);
    run_compute(start_cycles, end_cycles, checksum, loop_count);
    writeincr(out, static_cast<std::uint32_t>(1));
    writeincr(out, loop_count);
    write_record(out, 0, start_cycles, end_cycles, checksum);
}

void bf16_full_tile_middle(input_stream_uint32* in,
                           output_stream_uint32* out) {
    std::uint64_t start_cycles = 0;
    std::uint64_t end_cycles = 0;
    std::uint32_t checksum = 0;

    const std::uint32_t count = readincr(in);
    const std::uint32_t loop_count = readincr(in);
    run_compute(start_cycles, end_cycles, checksum, loop_count);
    writeincr(out, count + 1);
    writeincr(out, loop_count);
    forward_records(in, out, static_cast<int>(count));
    write_record(out, count, start_cycles, end_cycles, checksum);
}

void bf16_full_tile_last(input_stream_uint32* in,
                         output_stream_uint32* out) {
    bf16_full_tile_middle(in, out);
    for (int i = 0; i < peak_bf16::StreamPaddingWords; ++i) {
        writeincr(out, static_cast<std::uint32_t>(0));
    }
}
