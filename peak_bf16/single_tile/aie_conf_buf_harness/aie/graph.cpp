#include "graph.h"

PeakBf16HarnessGraph gr_peak_bf16;

#if defined(__AIESIM__) || defined(__X86SIM__)
int main(int argc, char** argv) {
    gr_peak_bf16.init();
    gr_peak_bf16.run(1);
    gr_peak_bf16.end();
    return 0;
}
#endif
