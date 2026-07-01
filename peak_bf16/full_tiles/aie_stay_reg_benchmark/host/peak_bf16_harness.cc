#include "peak_bf16_harness.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

#include "peak_bf16_config.h"
#include "test_harness_mgr_base.hpp"
#include "test_harness_mgr_client.hpp"
#include "test_harness_sockets.hpp"

namespace peak_bf16 {
namespace {

constexpr const char* XclbinPath = "vek280_test_harness.xclbin";
constexpr unsigned int GraphIndex = 0;
constexpr unsigned int GraphIterations = 1;

struct BenchmarkConfig {
    const char* profile = "medium-size";
    int input_m = MediumM;
    int input_k = MediumK;
    int input_n = MediumN;
    std::uint32_t loop_count = MediumLoopCount;
    std::uint64_t flops_per_tile_run = 0;
    std::uint64_t flops_per_graph_run = 0;
};

struct TileRecord {
    int channel = 0;
    int local_id = 0;
    int tile_id = 0;
    std::uint32_t checksum = 0;
    std::uint64_t start_cycles = 0;
    std::uint64_t end_cycles = 0;
    std::uint64_t delta_cycles = 0;
};

std::uint64_t make_u64(std::uint32_t lo, std::uint32_t hi) {
    return static_cast<std::uint64_t>(lo) |
           (static_cast<std::uint64_t>(hi) << 32);
}

BenchmarkConfig make_benchmark_config(const char* profile_name) {
    BenchmarkConfig cfg;
    if (profile_name == nullptr ||
        std::strcmp(profile_name, "medium-size") == 0) {
        cfg = {"medium-size", MediumM, MediumK, MediumN, MediumLoopCount, 0,
               0};
    } else if (std::strcmp(profile_name, "tiny-size") == 0) {
        cfg = {"tiny-size", TinyM, TinyK, TinyN, TinyLoopCount, 0, 0};
    } else if (std::strcmp(profile_name, "huge-size") == 0) {
        cfg = {"huge-size", HugeM, HugeK, HugeN, HugeLoopCount, 0, 0};
    } else {
        throw std::invalid_argument(
            "unsupported BENCHMARK_PROFILE. Use tiny-size, medium-size, or "
            "huge-size.");
    }

    cfg.flops_per_tile_run =
        2ull * static_cast<std::uint64_t>(cfg.input_m) * cfg.input_k *
        cfg.input_n;
    cfg.flops_per_graph_run =
        static_cast<std::uint64_t>(NumTiles) * cfg.flops_per_tile_run;
    return cfg;
}

std::vector<test_harness::test_harness_args> make_args(
    std::array<std::vector<std::uint32_t>, NumInputPlio>& control,
    OutputChannels& output) {
    std::vector<test_harness::test_harness_args> args;
    for (int ch = 0; ch < NumInputPlio; ++ch) {
        args.push_back({static_cast<test_harness::channel_index>(
                            test_harness::PLIO_01_TO_AIE + ch),
                        ControlBytesPerPlio,
                        static_cast<std::size_t>(1),
                        static_cast<std::uint64_t>(0),
                        reinterpret_cast<char*>(control[ch].data())});
    }
    for (int ch = 0; ch < NumOutputPlio; ++ch) {
        args.push_back({static_cast<test_harness::channel_index>(
                            test_harness::PLIO_01_FROM_AIE + ch),
                        OutputBytesPerPlio,
                        static_cast<std::size_t>(1),
                        static_cast<std::uint64_t>(0),
                        reinterpret_cast<char*>(output[ch].data())});
    }
    return args;
}

std::vector<TileRecord> parse_records(const OutputChannels& output,
                                      std::uint32_t expected_loop_count) {
    std::vector<TileRecord> records;
    records.reserve(NumTiles);

    for (int ch = 0; ch < NumOutputPlio; ++ch) {
        if (output[ch].empty()) {
            throw std::runtime_error("empty output channel.");
        }
        const std::uint32_t count = output[ch][0];
        if (count != static_cast<std::uint32_t>(TilesPerOutput)) {
            throw std::runtime_error("unexpected metadata record count.");
        }
        const std::uint32_t loop_count = output[ch][1];
        if (loop_count != expected_loop_count) {
            throw std::runtime_error("unexpected metadata loop count.");
        }

        for (int local = 0; local < TilesPerOutput; ++local) {
            const std::size_t base =
                StreamHeaderWords +
                static_cast<std::size_t>(local) * MetadataWords;
            const std::uint32_t data_flag = output[ch][base + 0];
            if (data_flag != DataFlag) {
                throw std::runtime_error("invalid metadata data flag.");
            }

            TileRecord r;
            r.channel = ch;
            r.local_id = static_cast<int>(output[ch][base + 1]);
            r.tile_id = ch * TilesPerOutput + r.local_id;
            r.checksum = output[ch][base + 2];
            r.start_cycles = make_u64(output[ch][base + 3],
                                      output[ch][base + 4]);
            r.end_cycles = make_u64(output[ch][base + 5],
                                    output[ch][base + 6]);
            r.delta_cycles = r.end_cycles - r.start_cycles;
            records.push_back(r);
        }
    }
    return records;
}

void print_summary(const std::vector<TileRecord>& records,
                   const BenchmarkConfig& cfg) {
    if (records.size() != static_cast<std::size_t>(NumTiles)) {
        throw std::runtime_error("unexpected number of tile records.");
    }

    std::uint64_t min_delta = std::numeric_limits<std::uint64_t>::max();
    std::uint64_t max_delta = 0;
    long double sum_delta = 0.0L;

    for (const TileRecord& r : records) {
        min_delta = std::min(min_delta, r.delta_cycles);
        max_delta = std::max(max_delta, r.delta_cycles);
        sum_delta += static_cast<long double>(r.delta_cycles);
    }

    const long double avg_delta =
        sum_delta / static_cast<long double>(records.size());
    const std::uint64_t avg_delta_cycles =
        static_cast<std::uint64_t>(avg_delta + 0.5L);
    const double avg_tile_seconds =
        static_cast<double>(avg_delta) / static_cast<double>(AieClockHz);
    const double max_tile_seconds =
        static_cast<double>(max_delta) / static_cast<double>(AieClockHz);
    const double avg_delta_array_tflops =
        static_cast<double>(cfg.flops_per_graph_run) / avg_tile_seconds /
        1.0e12;
    const double max_delta_array_tflops =
        static_cast<double>(cfg.flops_per_graph_run) / max_tile_seconds /
        1.0e12;
    const double avg_tile_gflops =
        static_cast<double>(cfg.flops_per_tile_run) / avg_tile_seconds / 1.0e9;
    const double theoretical_tflops =
        static_cast<double>(NumTiles) * 128.0 * 2.0 *
        static_cast<double>(AieClockHz) / 1.0e12;
    const double avg_delta_efficiency =
        avg_delta_array_tflops / theoretical_tflops * 100.0;
    const double max_delta_efficiency =
        max_delta_array_tflops / theoretical_tflops * 100.0;

    std::cout << "AIE full-tile peak summary\n";
    std::cout << " - min delta cycles              : " << min_delta << '\n';
    std::cout << " - max delta cycles              : " << max_delta << '\n';
    std::cout << " - avg delta cycles              : "
              << avg_delta_cycles << '\n';
    std::cout << " - per-tile avg GFLOP/s          : " << avg_tile_gflops << '\n';
    std::cout << " - array TFLOP/s by avg tile     : "
              << avg_delta_array_tflops << '\n';
    std::cout << " - array TFLOP/s by slowest tile : "
              << max_delta_array_tflops << '\n';
    std::cout << " - theoretical TFLOP/s           : "
              << theoretical_tflops << '\n';
    std::cout << " - efficiency by avg tile        : "
              << avg_delta_efficiency << "%\n";
    std::cout << " - efficiency by slowest tile    : "
              << max_delta_efficiency << "%\n";
}

}  // namespace

void run_harness_bf16_full_tiles(OutputChannels& output) {
    const BenchmarkConfig cfg =
        make_benchmark_config(std::getenv("BENCHMARK_PROFILE"));

    std::array<std::vector<std::uint32_t>, NumInputPlio> control;
    for (int ch = 0; ch < NumInputPlio; ++ch) {
        control[ch].assign(ControlWordsPerPlio, 0);
        control[ch][0] = cfg.loop_count;
    }
    for (int ch = 0; ch < NumOutputPlio; ++ch) {
        output[ch].assign(OutputWordsPerPlio, 0);
    }

    std::cout << "Using XCLBIN file: " << XclbinPath << '\n';
    std::cout << "Running peak BF16 stay-reg full-tile through Test Harness\n";
    std::cout << " - Mode                          : 304-tile register-resident probe\n";
    std::cout << " - Benchmark profile             : " << cfg.profile << '\n';
    std::cout << " - Graph                         : " << GraphName << '\n';
    std::cout << " - Tile array                    : "
              << NumRows << "x" << NumCols << '\n';
    std::cout << " - AIE tiles                     : " << NumTiles << '\n';
    std::cout << " - Input GEMM shape per tile     : "
              << cfg.input_m << "x" << cfg.input_k << "x" << cfg.input_n
              << '\n';
    std::cout << " - Runtime loop count            : " << cfg.loop_count << '\n';
    std::cout << " - FLOPs per tile                : "
              << cfg.flops_per_tile_run << '\n';
    std::cout << " - FLOPs per graph iteration     : "
              << cfg.flops_per_graph_run << '\n';

    test_harness::test_harness_mgr_client mgr(XclbinPath,
                                              {GraphName},
                                              "vek280");

    mgr.runAIEGraph(GraphIndex, GraphIterations);
    mgr.runTestHarness(test_harness::FUNC_MODE, make_args(control, output));
    mgr.waitForRes(0);

    if (!mgr.isResultValid()) {
        throw std::runtime_error("test harness transaction is not valid.");
    }

    print_summary(parse_records(output, cfg.loop_count), cfg);
}

}  // namespace peak_bf16
