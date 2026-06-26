#include "graph.h"

PeakBf16SingleTileGraph peak_bf16_single_tile_graph;

#if defined(__AIESIM__) || defined(__X86SIM__)
int main(int argc, char** argv) {
    peak_bf16_single_tile_graph.init();
    peak_bf16_single_tile_graph.run(1);
    peak_bf16_single_tile_graph.end();
    return 0;
}
#endif
