#include "graph.h"

PeakInt8HarnessGraph gr_peak_int8;

#if defined(__AIESIM__) || defined(__X86SIM__)
int main(int argc, char** argv) {
    gr_peak_int8.init();
    gr_peak_int8.run(1);
    gr_peak_int8.end();
    return 0;
}
#endif
