#include <xrt/xrt_bo.h>
#include <xrt/xrt_device.h>
#include <xrt/xrt_graph.h>
#include <xrt/xrt_kernel.h>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>

#include "peak_int8_config.h"

namespace {

bool wait_run(const char* name, const xrt::run& run) {
    const auto state = run.wait(std::chrono::milliseconds(60000));
    if (state == ERT_CMD_STATE_TIMEOUT) {
        std::printf("%s timed out\n", name);
        return false;
    }
    std::printf("%s completed with state %d\n", name, static_cast<int>(state));
    return true;
}

std::uint64_t parse_repeats(int argc, char** argv) {
    if (argc >= 3) {
        const auto parsed = std::strtoull(argv[2], nullptr, 0);
        return parsed == 0 ? 1ull : parsed;
    }
    return 10000000ull;
}

}  // namespace

int run(int argc, char** argv) {
    if (argc < 2 || argc > 3) {
        std::printf("Usage: %s <peak_int8.xclbin> [repeats]\n", argv[0]);
        return 1;
    }

    const std::string xclbin_path = argv[1];
    const std::uint64_t repeats = parse_repeats(argc, argv);

    std::printf("Loading xclbin: %s\n", xclbin_path.c_str());
    auto device = xrt::device(0);
    auto uuid = device.load_xclbin(xclbin_path);

    auto mm2s = xrt::kernel(device, uuid, "mm2s:{mm2s_1}");
    auto s2mm = xrt::kernel(device, uuid, "s2mm:{s2mm_1}");
    auto graph = xrt::graph(device, uuid, peak_int8::GraphName);

    constexpr std::size_t InputBytes  = peak_int8::InputWords  * sizeof(std::uint32_t);
    constexpr std::size_t OutputBytes = peak_int8::OutputWords * sizeof(std::uint32_t);

    auto input_bo  = xrt::bo(device, InputBytes,  xrt::bo::flags::normal, mm2s.group_id(0));
    auto output_bo = xrt::bo(device, OutputBytes, xrt::bo::flags::normal, s2mm.group_id(0));

    auto* input  = input_bo.map<std::uint32_t*>();
    auto* output = output_bo.map<std::uint32_t*>();
    input[0] = static_cast<std::uint32_t>(repeats);
    for (int i = 0; i < peak_int8::OutputWords; ++i) {
        output[i] = 0;
    }

    input_bo.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    output_bo.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    std::printf("\n");
    std::printf("============================================================\n");
    std::printf("  Peak INT8 GEMM Benchmark — VEK280 AIE-ML\n");
    std::printf("============================================================\n");
    std::printf("  AIE clock          : 312.5 MHz\n");
    std::printf("  MMUL shape         : M=%d, K=%d, N=%d\n",
                peak_int8::M, peak_int8::K, peak_int8::N);
    std::printf("  MACs / mmul call   : %d\n", peak_int8::MacsPerMmul);
    std::printf("  Unroll MACs/iter   : %d  (2 x %d mmul units)\n",
                2 * peak_int8::MacsPerMmul, peak_int8::MacsPerMmul);
    std::printf("  OPS / iteration    : %d\n", peak_int8::MacsPerMmul * 2 * 2);
    std::printf("  Runtime repeats    : %llu\n",
                static_cast<unsigned long long>(repeats));
    std::printf("============================================================\n");
    std::fflush(stdout);

    auto mm2s_run = mm2s(input_bo, nullptr, peak_int8::InputWords);
    auto s2mm_run = s2mm(output_bo, nullptr, peak_int8::OutputWords);

    const auto t0 = std::chrono::high_resolution_clock::now();
    graph.run(1);
    graph.end();
    const auto t1 = std::chrono::high_resolution_clock::now();

    if (!wait_run("mm2s", mm2s_run)) return 2;
    if (!wait_run("s2mm", s2mm_run)) return 3;

    output_bo.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

    const double seconds = std::chrono::duration<double>(t1 - t0).count();

    const std::uint64_t kernel_repeats = output[0];
    const std::uint64_t num_mmul       = output[1];
    const std::uint64_t macs_per_mmul  = output[2];
    const std::uint64_t total_macs =
        kernel_repeats * num_mmul * macs_per_mmul;
    const std::uint64_t total_ops =
        total_macs * peak_int8::IntOpsPerMac;  // multiply + add

    std::printf("\n");
    std::printf("--- AIE Verification ---\n");
    std::printf("  repeats echoed      : %llu\n", static_cast<unsigned long long>(kernel_repeats));
    std::printf("  mmul units         : %llu\n", static_cast<unsigned long long>(num_mmul));
    std::printf("  MACs/mmul          : %llu\n", static_cast<unsigned long long>(macs_per_mmul));
    std::printf("  checksum (i32)     : %d\n", output[3]);
    std::printf("  magic              : 0x%08x\n", output[4]);

    std::printf("\n--- Performance ---\n");
    std::printf("  Wall time          : %.9f s\n", seconds);
    std::printf("  Total MACs         : %llu\n",  static_cast<unsigned long long>(total_macs));
    std::printf("  Total OPS          : %.3e\n", static_cast<double>(total_ops));
    std::printf("  Throughput          :\n");
    std::printf("    GMAC/s            : %.3f\n",
                static_cast<double>(total_macs) / seconds / 1.0e9);
    std::printf("    GOPS (×10^9)     : %.3f\n",
                static_cast<double>(total_ops)  / seconds / 1.0e9);
    std::printf("    TOPS (×10^12)    : %.4f\n",
                static_cast<double>(total_ops)  / seconds / 1.0e12);

    if (output[4] != 0x494E5438u) {
        std::printf("\nFAIL: invalid magic (expected 0x494E5438)\n");
        return 4;
    }
    if (kernel_repeats == 0 || output[3] == 0) {
        std::printf("\nFAIL: zero repeats or checksum\n");
        return 4;
    }

    std::printf("\n");
    std::printf("============================================================\n");
    std::printf("  PASS — Peak INT8 benchmark complete\n");
    std::printf("============================================================\n");

    return 0;
}

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const std::exception& e) {
        std::printf("Exception: %s\n", e.what());
        return 1;
    }
}
