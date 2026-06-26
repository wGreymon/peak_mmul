#include "peak_int8_harness.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "peak_int8_config.h"
#include "stx_host_utils.hpp"
#include "test_harness_mgr_base.hpp"
#include "test_harness_mgr_client.hpp"
#include "test_harness_sockets.hpp"

namespace peak_int8 {
namespace {

constexpr const char* XclbinPath = "vek280_test_harness.xclbin";
constexpr unsigned int GraphIndex = 0;

std::uint32_t load_u32(const std::uint8_t* bytes) {
    std::uint32_t value = 0;
    std::memcpy(&value, bytes, sizeof(value));
    return value;
}

std::uint64_t make_u64(std::uint32_t lo, std::uint32_t hi) {
    return static_cast<std::uint64_t>(lo) |
           (static_cast<std::uint64_t>(hi) << 32);
}

void print_aie_cycle_metadata(const std::uint8_t* output,
                              std::size_t graph_iterations) {
    const std::uint8_t* perf =
        output + (graph_iterations - 1) * CElements + PerfByteOffset;

    const std::uint32_t magic = load_u32(perf + 0);
    const std::uint32_t version = load_u32(perf + 4);
    const std::uint64_t start_cycles =
        make_u64(load_u32(perf + 8), load_u32(perf + 12));
    const std::uint64_t end_cycles =
        make_u64(load_u32(perf + 16), load_u32(perf + 20));
    const std::uint64_t delta_cycles =
        make_u64(load_u32(perf + 24), load_u32(perf + 28));
    const bool valid_perf =
        magic == PerfMagic && version == PerfVersion && delta_cycles != 0;
    const double aie_seconds =
        static_cast<double>(delta_cycles) / static_cast<double>(AieClockHz);
    const std::uint64_t total_macs =
        static_cast<std::uint64_t>(MacsPerGraphRun);
    const std::uint64_t total_ops =
        static_cast<std::uint64_t>(OpsPerGraphRun);

    std::cout << "AIE kernel cycle metadata\n";
    std::cout << " - valid                         : "
              << (valid_perf ? "yes" : "no") << '\n';
    std::cout << " - metadata byte offset          : "
              << PerfByteOffset << '\n';
    std::cout << " - magic/version                 : 0x"
              << std::hex << magic << " / " << std::dec << version << '\n';
    std::cout << " - start cycles                  : " << start_cycles << '\n';
    std::cout << " - end cycles                    : " << end_cycles << '\n';
    std::cout << " - delta cycles                  : " << delta_cycles << '\n';
    std::cout << " - assumed AIE clock Hz          : " << AieClockHz << '\n';
    if (valid_perf) {
        std::cout << " - AIE compute seconds           : " << aie_seconds << '\n';
        std::cout << " - AIE-only GMAC/s               : "
                  << static_cast<double>(total_macs) / aie_seconds / 1.0e9
                  << '\n';
        std::cout << " - AIE-only GOPS                 : "
                  << static_cast<double>(total_ops) / aie_seconds / 1.0e9
                  << '\n';
        std::cout << " - AIE-only TOPS                 : "
                  << static_cast<double>(total_ops) / aie_seconds / 1.0e12
                  << '\n';
    }
}

std::vector<test_harness::test_harness_args> make_args(
    std::vector<std::int8_t>& input_a,
    std::vector<std::int8_t>& input_b,
    std::vector<std::uint8_t>& output,
    int repetitions,
    int channel_delay) {
    std::vector<test_harness::test_harness_args> args;
    args.push_back({test_harness::PLIO_01_TO_AIE,
                    AInputBytes,
                    static_cast<std::size_t>(repetitions),
                    static_cast<std::uint64_t>(channel_delay),
                    reinterpret_cast<char*>(input_a.data())});
    args.push_back({test_harness::PLIO_02_TO_AIE,
                    BInputBytes,
                    static_cast<std::size_t>(repetitions),
                    static_cast<std::uint64_t>(channel_delay),
                    reinterpret_cast<char*>(input_b.data())});
    args.push_back({test_harness::PLIO_01_FROM_AIE,
                    COutputBytes,
                    static_cast<std::size_t>(repetitions),
                    static_cast<std::uint64_t>(channel_delay),
                    reinterpret_cast<char*>(output.data())});
    return args;
}

}  // namespace

void run_harness_int8(std::uint8_t* output,
                      const std::int8_t* input_a,
                      const std::int8_t* input_b,
                      std::size_t graph_iterations) {
    if (output == nullptr || input_a == nullptr || input_b == nullptr) {
        throw std::invalid_argument("run_harness_int8 received null pointer.");
    }
    if (graph_iterations == 0) {
        throw std::invalid_argument("graph_iterations must be positive.");
    }

    const stx::ops::HarnessRunOptions run_options =
        stx::ops::get_harness_run_options();
    const stx::ops::FuncModeOptions& func_mode = run_options.func_mode;

    std::vector<std::int8_t> padded_input_a(AElements * graph_iterations);
    std::vector<std::int8_t> padded_input_b(BElements * graph_iterations);
    std::vector<std::uint8_t> padded_output(CElements * graph_iterations);

    for (std::size_t i = 0; i < graph_iterations; ++i) {
        std::copy_n(input_a, AElements, padded_input_a.data() + i * AElements);
        std::copy_n(input_b, BElements, padded_input_b.data() + i * BElements);
    }

    std::cout << "Using XCLBIN file: " << XclbinPath << '\n';
    std::cout << "Running peak INT8 stay-reg single-tile through Test Harness\n";
    std::cout << " - Mode                          : register-resident compute probe\n";
    std::cout << " - Graph                         : " << GraphName << '\n';
    std::cout << " - Input GEMM shape              : "
              << InputM << "x" << InputK << "x" << InputN << '\n';
    std::cout << " - MMUL shape                    : "
              << MmulM << "x" << MmulK << "x" << MmulN << " int8\n";
    std::cout << " - A input bytes/iteration       : " << AInputBytes << '\n';
    std::cout << " - B input bytes/iteration       : " << BInputBytes << '\n';
    std::cout << " - C output bytes/iteration      : " << COutputBytes << '\n';
    std::cout << " - C result bytes/iteration      : " << CResultElements << '\n';
    std::cout << " - perf metadata bytes           : " << PerfOutputBytes << '\n';
    std::cout << " - Graph iterations              : " << graph_iterations << '\n';
    std::cout << " - OPS per graph iteration       : " << OpsPerGraphRun << '\n';
    std::cout << " - Correctness check             : disabled\n";
    std::cout << " - Channel delay                 : "
              << func_mode.channel_delay << " cycles\n";
    std::cout << " - Run performance mode          : "
              << (run_options.run_perf_mode ? "yes" : "no") << '\n';
    if (run_options.run_perf_mode) {
        std::cout << " - Performance repetitions       : "
                  << run_options.perf_repetitions << '\n';
    }

    test_harness::test_harness_mgr_client mgr(XclbinPath,
                                              {GraphName},
                                              "vek280");

    std::cout << "Testing Function mode." << std::endl;
    const auto aie_t0 = std::chrono::high_resolution_clock::now();
    mgr.runAIEGraph(GraphIndex, static_cast<unsigned int>(graph_iterations));
    mgr.runTestHarness(test_harness::FUNC_MODE,
                       make_args(padded_input_a,
                                 padded_input_b,
                                 padded_output,
                                 func_mode.num_repetitions,
                                 func_mode.channel_delay));
    mgr.waitForRes(0);
    const auto aie_t1 = std::chrono::high_resolution_clock::now();

    mgr.printPerf();
    const double aie_us =
        std::chrono::duration<double, std::micro>(aie_t1 - aie_t0).count();
    std::cout << " - AIE host end-to-end latency   : " << aie_us << " us\n";

    if (!mgr.isResultValid()) {
        throw std::runtime_error("test harness transaction is not valid.");
    }

    std::copy_n(padded_output.data(), CElements, output);
    print_aie_cycle_metadata(padded_output.data(), graph_iterations);

    if (run_options.run_perf_mode) {
        std::cout << "Testing Performance mode." << std::endl;
        const unsigned int perf_iterations =
            stx::ops::perf_graph_iterations(graph_iterations,
                                            run_options.perf_repetitions);
        const auto perf_t0 = std::chrono::high_resolution_clock::now();
        mgr.runAIEGraph(GraphIndex, perf_iterations);
        mgr.runTestHarness(test_harness::PERF_MODE,
                           make_args(padded_input_a,
                                     padded_input_b,
                                     padded_output,
                                     run_options.perf_repetitions,
                                     func_mode.channel_delay));
        mgr.waitForRes(0);
        const auto perf_t1 = std::chrono::high_resolution_clock::now();

        mgr.printPerf();
        const double perf_us =
            std::chrono::duration<double, std::micro>(perf_t1 - perf_t0).count();
        std::cout << " - AIE performance-mode latency  : " << perf_us << " us\n";
    }
}

}  // namespace peak_int8
