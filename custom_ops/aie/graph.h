#pragma once

#include <adf.h>

#include "kernels.h"
#include "peak_config.h"

class PeakMmulGraph : public adf::graph {
private:
    adf::kernel peak_kernel;

public:
    adf::input_plio in;
    adf::output_plio out;

    PeakMmulGraph() {
        peak_kernel = adf::kernel::create(peak_mmul_kernel);

        in = adf::input_plio::create(peak_mmul::PlioInName,
                                     adf::plio_32_bits);
        out = adf::output_plio::create(peak_mmul::PlioOutName,
                                       adf::plio_32_bits);

        adf::connect<adf::stream>(in.out[0], peak_kernel.in[0]);
        adf::connect<adf::stream>(peak_kernel.out[0], out.in[0]);

        adf::source(peak_kernel) = "aie/peak_mmul_kernel.cc";
        adf::runtime<adf::ratio>(peak_kernel) = 1.0;
    }
};

extern PeakMmulGraph peak_mmul_graph;

