#include "graph.h"

PeakMmulGraph peak_mmul_graph;

#if defined(__AIESIM__) || defined(__X86SIM__)
int main(int argc, char** argv) {
    peak_mmul_graph.init();
    peak_mmul_graph.run(1);
    peak_mmul_graph.end();
    return 0;
}
#endif

