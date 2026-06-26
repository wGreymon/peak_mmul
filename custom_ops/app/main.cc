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

#include "peak_config.h"

namespace {

bool wait_run(const char* name, const xrt::run& run) {
    const auto state = run.wait(std::chrono::milliseconds(10000));
    if (state == ERT_CMD_STATE_TIMEOUT) {
        std::printf("%s timed out\n", name);
        return false;
    }
    std::printf("%s completed with state %d\n", name, static_cast<int>(state));
    return true;
}

std::uint32_t parse_repeats(int argc, char** argv) {
    if (argc >= 3) {
        const auto parsed = std::strtoull(argv[2], nullptr, 0);
        return parsed == 0 ? 1u : static_cast<std::uint32_t>(parsed);
    }
    return 1000000u;
}

int run(int argc, char** argv) {
    if (argc < 2 || argc > 3) {
        std::printf("Usage: %s <peak_mmul.xclbin> [repeats]\n", argv[0]);
        return 1;
    }

    const std::string xclbin_path = argv[1];
    const std::uint32_t repeats = parse_repeats(argc, argv);

    std::printf("Loading xclbin: %s\n", xclbin_path.c_str());
    auto device = xrt::device(0);
    auto uuid = device.load_xclbin(xclbin_path);

    auto mm2s = xrt::kernel(device, uuid, "mm2s:{mm2s_1}");
    auto s2mm = xrt::kernel(device, uuid, "s2mm:{s2mm_1}");
    auto graph = xrt::graph(device, uuid, peak_mmul::GraphName);

    auto input_bo = xrt::bo(device, peak_mmul::InputBytes,
                            xrt::bo::flags::normal, mm2s.group_id(0));
    auto output_bo = xrt::bo(device, peak_mmul::OutputBytes,
                             xrt::bo::flags::normal, s2mm.group_id(0));

    auto* input = input_bo.map<std::uint32_t*>();
    auto* output = output_bo.map<std::uint32_t*>();
    input[0] = repeats;
    for (int i = 0; i < peak_mmul::OutputWords; ++i) {
        output[i] = 0;
    }

    input_bo.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    output_bo.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    std::printf("Peak MMUL benchmark\n");
    std::printf(" - MMUL shape             : %dx%dx%d bf16\n",
                peak_mmul::MmulM, peak_mmul::MmulK, peak_mmul::MmulN);
    std::printf(" - MACs per MMUL          : %d\n", peak_mmul::MacsPerMmul);
    std::printf(" - FLOPs per MMUL         : %d\n", peak_mmul::FlopsPerMmul);
    std::printf(" - Runtime repeats        : %u\n", repeats);
    std::printf(" - Compile unroll MACs    : 64\n");
    std::fflush(stdout);

    auto mm2s_run = mm2s(input_bo, nullptr, peak_mmul::InputWords);
    auto s2mm_run = s2mm(output_bo, nullptr, peak_mmul::OutputWords);

    const auto t0 = std::chrono::high_resolution_clock::now();
    graph.run(1);
    graph.end();
    const auto t1 = std::chrono::high_resolution_clock::now();

    if (!wait_run("mm2s", mm2s_run)) {
        return 2;
    }
    if (!wait_run("s2mm", s2mm_run)) {
        return 3;
    }

    output_bo.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

    const double seconds =
        std::chrono::duration<double>(t1 - t0).count();
    const std::uint64_t kernel_repeats = output[0];
    const std::uint64_t unroll_macs = output[1];
    const std::uint64_t macs_per_mmul = output[2];
    const std::uint64_t total_macs =
        kernel_repeats * unroll_macs * macs_per_mmul;
    const std::uint64_t total_flops = 2ull * total_macs;

    union {
        std::uint32_t u;
        float f;
    } checksum;
    checksum.u = output[3];

    std::printf("AIE output\n");
    std::printf(" - repeats echoed         : %u\n", output[0]);
    std::printf(" - unroll MACs echoed     : %u\n", output[1]);
    std::printf(" - MACs/MMUL echoed       : %u\n", output[2]);
    std::printf(" - checksum               : %.8e\n", checksum.f);
    std::printf(" - magic                  : 0x%08x\n", output[4]);

    std::printf("Measured host graph interval\n");
    std::printf(" - seconds                : %.9f\n", seconds);
    std::printf(" - total MACs             : %llu\n",
                static_cast<unsigned long long>(total_macs));
    std::printf(" - total FLOPs            : %llu\n",
                static_cast<unsigned long long>(total_flops));
    std::printf(" - GMAC/s                 : %.3f\n",
                static_cast<double>(total_macs) / seconds / 1.0e9);
    std::printf(" - GFLOP/s                : %.3f\n",
                static_cast<double>(total_flops) / seconds / 1.0e9);

    if (output[4] != 0x5045414bu || output[0] == 0 || checksum.f == 0.0f) {
        std::printf("FAIL: invalid output signature\n");
        return 4;
    }

    std::printf("PASS\n");
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const std::exception& e) {
        std::printf("Exception: %s\n", e.what());
        return 1;
    }
}

