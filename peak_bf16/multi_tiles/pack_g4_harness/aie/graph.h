#pragma once

#include <adf.h>
#include <string>

#include "kernels.h"
#include "peak_bf16_config.h"
#include "test_harness_graph.hpp"

class PeakBf16PackHarnessGraph : public adf::graph {
private:
    adf::kernel pack_kernel[peak_bf16::NumKernels];

public:
    adf::input_plio in_a[peak_bf16::NumAInputs];
    adf::input_plio in_b[peak_bf16::NumBInputs];
    adf::output_plio out_c;

    PeakBf16PackHarnessGraph() {
        for (int i = 0; i < peak_bf16::NumKernels; ++i) {
            if (i == 0) {
                pack_kernel[i] = adf::kernel::create(bf16_pack_first);
                adf::source(pack_kernel[i]) = "bf16_pack_first.cc";
            } else if (i == peak_bf16::NumKernels - 1) {
                pack_kernel[i] = adf::kernel::create(bf16_pack_last);
                adf::source(pack_kernel[i]) = "bf16_pack_last.cc";
            } else {
                pack_kernel[i] = adf::kernel::create(bf16_pack_middle);
                adf::source(pack_kernel[i]) = "bf16_pack_middle.cc";
            }
            adf::runtime<adf::ratio>(pack_kernel[i]) = 1.0;
            adf::location<adf::kernel>(pack_kernel[i]) = adf::tile(i, 0);
        }

        for (int i = 0; i < peak_bf16::NumAInputs; ++i) {
            in_a[i] = adf::input_plio::create(
                "PLIO_0" + std::to_string(i + 1) + "_TO_AIE",
                adf::plio_128_bits,
                "data/A" + std::to_string(i) + ".txt");
        }
        for (int i = 0; i < peak_bf16::NumBInputs; ++i) {
            in_b[i] = adf::input_plio::create(
                "PLIO_0" + std::to_string(peak_bf16::NumAInputs + i + 1) +
                    "_TO_AIE",
                adf::plio_128_bits,
                "data/B" + std::to_string(i) + ".txt");
        }
        out_c = adf::output_plio::create("PLIO_01_FROM_AIE",
                                         adf::plio_128_bits,
                                         "data/C0.txt");

        for (int i = 0; i < peak_bf16::NumKernels; ++i) {
            adf::connect<>(in_a[i].out[0], pack_kernel[i].in[0]);
            adf::dimensions(pack_kernel[i].in[0]) = {peak_bf16::AElements};

            adf::connect<>(in_b[i].out[0], pack_kernel[i].in[1]);
            adf::dimensions(pack_kernel[i].in[1]) = {peak_bf16::BElements};

            adf::location<adf::stack>(pack_kernel[i]) =
                adf::location<adf::kernel>(pack_kernel[i]);
            adf::location<adf::buffer>(pack_kernel[i].in[0]) =
                adf::location<adf::kernel>(pack_kernel[i]);
            adf::location<adf::buffer>(pack_kernel[i].in[1]) =
                adf::location<adf::kernel>(pack_kernel[i]);
        }

        for (int i = 0; i < peak_bf16::NumKernels - 1; ++i) {
            adf::connect<adf::cascade>(pack_kernel[i].out[0],
                                       pack_kernel[i + 1].in[2]);
        }

        adf::connect<>(pack_kernel[peak_bf16::NumKernels - 1].out[0],
                       out_c.in[0]);
        adf::dimensions(pack_kernel[peak_bf16::NumKernels - 1].out[0]) = {
            peak_bf16::CElements};
        adf::location<adf::buffer>(
            pack_kernel[peak_bf16::NumKernels - 1].out[0]) =
            adf::location<adf::kernel>(
                pack_kernel[peak_bf16::NumKernels - 1]);
    }
};

extern PeakBf16PackHarnessGraph gr_peak_bf16;
