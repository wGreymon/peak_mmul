#pragma once

#include <adf.h>

#include "kernels.h"
#include "peak_int8_config.h"
#include "test_harness_graph.hpp"

class PeakInt8HarnessGraph : public adf::graph {
private:
    adf::kernel peak_kernel;

public:
    adf::input_plio in_a;
    adf::input_plio in_b;
    adf::output_plio out;

    PeakInt8HarnessGraph() {
        peak_kernel = adf::kernel::create(int8_stay_reg_kernel);

        in_a = adf::input_plio::create(peak_int8::APlioInName,
                                       adf::plio_128_bits,
                                       "data/DataIn0.txt");
        in_b = adf::input_plio::create(peak_int8::BPlioInName,
                                       adf::plio_128_bits,
                                       "data/DataIn1.txt");
        out = adf::output_plio::create(peak_int8::PlioOutName,
                                       adf::plio_128_bits,
                                       "data/DataOut0.txt");

        adf::connect<>(in_a.out[0], peak_kernel.in[0]);
        adf::dimensions(peak_kernel.in[0]) = {peak_int8::AElements};

        adf::connect<>(in_b.out[0], peak_kernel.in[1]);
        adf::dimensions(peak_kernel.in[1]) = {peak_int8::BElements};

        adf::connect<>(peak_kernel.out[0], out.in[0]);
        adf::dimensions(peak_kernel.out[0]) = {peak_int8::CElements};

        adf::source(peak_kernel) = "int8_stay_reg_kernel.cc";
        adf::runtime<adf::ratio>(peak_kernel) = 1.0;

        adf::location<adf::buffer>(peak_kernel.in[0]) =
            adf::location<adf::kernel>(peak_kernel);
        adf::location<adf::buffer>(peak_kernel.in[1]) =
            adf::location<adf::kernel>(peak_kernel);
        adf::location<adf::buffer>(peak_kernel.out[0]) =
            adf::location<adf::kernel>(peak_kernel);
    }
};

extern PeakInt8HarnessGraph gr_peak_int8;
