#include "graph.h"

PeakBf16FullTilesGraph gr_peak_bf16_full_tiles;

#if defined(__AIESIM__) || defined(__X86SIM__)
int main(int argc, char** argv) {
    gr_peak_bf16_full_tiles.init();
    gr_peak_bf16_full_tiles.run(1);
    gr_peak_bf16_full_tiles.end();
    return 0;
}
#endif
