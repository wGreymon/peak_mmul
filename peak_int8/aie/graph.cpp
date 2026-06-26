#include "graph.h"

PeakInt8Graph peak_int8_graph;

#if defined(__AIESIM__) || defined(__X86SIM__)
int main(int argc, char** argv) {
    peak_int8_graph.init();
    peak_int8_graph.run(1);
    peak_int8_graph.end();
    return 0;
}
#endif
