#pragma once

#include <adf.h>

#include "peak_int8_kernel.cc"
#include "peak_int8_config.h"

class PeakInt8Graph : public adf::graph {
private:
    adf::kernel peak_kernel;

public:
    adf::input_plio in;
    adf::output_plio out;

    PeakInt8Graph() {
        peak_kernel = adf::kernel::create(peak_int8_kernel);

        in  = adf::input_plio::create(peak_int8::PlioInName,  adf::plio_32_bits);
        out = adf::output_plio::create(peak_int8::PlioOutName, adf::plio_32_bits);

        adf::connect<adf::stream>(in.out[0],  peak_kernel.in[0]);
        adf::connect<adf::stream>(peak_kernel.out[0], out.in[0]);

        adf::source(peak_kernel) = "aie/peak_int8_kernel.cc";
        adf::runtime<adf::ratio>(peak_kernel) = 1.0;
    }
};

extern PeakInt8Graph peak_int8_graph;
