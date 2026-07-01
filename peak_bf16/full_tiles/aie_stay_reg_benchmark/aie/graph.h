#pragma once

#include <adf.h>
#include <string>
#include <vector>

#include "kernels.h"
#include "peak_bf16_config.h"

class PeakBf16FullTilesGraph : public adf::graph {
private:
    adf::kernel tile_kernel[peak_bf16::NumTiles];

    static std::string two_digit(int value) {
        return value < 10 ? "0" + std::to_string(value)
                          : std::to_string(value);
    }

public:
    adf::input_plio control[peak_bf16::NumInputPlio];
    adf::output_plio out[peak_bf16::NumOutputPlio];

    PeakBf16FullTilesGraph() {
        for (int ch = 0; ch < peak_bf16::NumInputPlio; ++ch) {
            control[ch] = adf::input_plio::create(
                "PLIO_" + two_digit(ch + 1) + "_TO_AIE",
                adf::plio_128_bits,
                "data/Control" + std::to_string(ch) + ".txt");
        }

        for (int ch = 0; ch < peak_bf16::NumOutputPlio; ++ch) {
            out[ch] = adf::output_plio::create(
                "PLIO_" + two_digit(ch + 1) + "_FROM_AIE",
                adf::plio_128_bits,
                "data/Cycles" + std::to_string(ch) + ".txt");
        }

        // 304 tile / 16 
        for (int ch = 0; ch < peak_bf16::NumOutputPlio; ++ch) {
            for (int stage = 0; stage < peak_bf16::TilesPerOutput; ++stage) {
                const int idx = ch * peak_bf16::TilesPerOutput + stage;
                if (stage == 0) {
                    tile_kernel[idx] =
                        adf::kernel::create(bf16_full_tile_first);
                } else if (stage == peak_bf16::TilesPerOutput - 1) {
                    tile_kernel[idx] =
                        adf::kernel::create(bf16_full_tile_last);
                } else {
                    tile_kernel[idx] =
                        adf::kernel::create(bf16_full_tile_middle);
                }
                adf::source(tile_kernel[idx]) = "bf16_full_tile_kernel.cc";
                adf::runtime<adf::ratio>(tile_kernel[idx]) = 1.0;

                const int row = ch / 2;
                const int col = (ch % 2) * peak_bf16::TilesPerOutput + stage;
                adf::location<adf::kernel>(tile_kernel[idx]) = adf::tile(col, row);
                adf::location<adf::stack>(tile_kernel[idx]) =
                    adf::location<adf::kernel>(tile_kernel[idx]);    // 把该kernel所需的栈空间也放在同一个tile上
            }

            const int head = ch * peak_bf16::TilesPerOutput;
            adf::connect<adf::stream>(control[ch].out[0],
                                      tile_kernel[head].in[0]);

            for (int stage = 0; stage < peak_bf16::TilesPerOutput - 1; ++stage) {
                const int src = ch * peak_bf16::TilesPerOutput + stage;
                const int dst = src + 1;
                adf::connect<adf::stream>(tile_kernel[src].out[0],
                                          tile_kernel[dst].in[0]);
            }

            const int tail =
                ch * peak_bf16::TilesPerOutput + peak_bf16::TilesPerOutput - 1;
            adf::connect<adf::stream>(tile_kernel[tail].out[0], out[ch].in[0]);
        }
    }
};

extern PeakBf16FullTilesGraph gr_peak_bf16_full_tiles;
