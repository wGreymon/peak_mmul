#include <xrt/xrt_bo.h>
#include <xrt/xrt_device.h>
#include <xrt/xrt_graph.h>
#include <xrt/xrt_kernel.h>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <string>

#include "peak_bf16_config.h"

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

std::uint16_t float_to_bf16(float value) {
    union {
        float f;
        std::uint32_t u;
    } bits{};
    bits.f = value;

    const std::uint32_t lsb = (bits.u >> 16) & 1u;
    const std::uint32_t rounding_bias = 0x7fffu + lsb;
    return static_cast<std::uint16_t>((bits.u + rounding_bias) >> 16);
}

std::uint32_t pack_bf16(std::uint16_t lo, std::uint16_t hi) {
    return static_cast<std::uint32_t>(lo) |
           (static_cast<std::uint32_t>(hi) << 16);
}

void fill_a(std::uint32_t* words) {
    for (int i = 0; i < peak_bf16::AInputWords; ++i) {
        const int e0 = 2 * i;
        const int e1 = e0 + 1;
        const float v0 = static_cast<float>((e0 & 7) + 1) * 0.125f;
        const float v1 = static_cast<float>((e1 & 7) + 1) * 0.125f;
        words[i] = pack_bf16(float_to_bf16(v0), float_to_bf16(v1));
    }
}

void fill_b(std::uint32_t* words) {
    for (int i = 0; i < peak_bf16::BInputWords; ++i) {
        const int e0 = 2 * i;
        const int e1 = e0 + 1;
        const float v0 = static_cast<float>(((e0 * 3) & 7) + 1) * 0.0625f;
        const float v1 = static_cast<float>(((e1 * 3) & 7) + 1) * 0.0625f;
        words[i] = pack_bf16(float_to_bf16(v0), float_to_bf16(v1));
    }
}

int run(int argc, char** argv) {
    if (argc != 2) {
        std::printf("Usage: %s <peak_bf16_single_tile.xclbin>\n", argv[0]);
        return 1;
    }

    const std::string xclbin_path = argv[1];

    std::printf("Loading xclbin: %s\n", xclbin_path.c_str());
    auto device = xrt::device(0);
    auto uuid = device.load_xclbin(xclbin_path);

    auto mm2s_a = xrt::kernel(device, uuid, "mm2s:{mm2s_1}");
    auto mm2s_b = xrt::kernel(device, uuid, "mm2s:{mm2s_2}");
    auto s2mm = xrt::kernel(device, uuid, "s2mm:{s2mm_1}");
    auto graph = xrt::graph(device, uuid, peak_bf16::GraphName);

    auto a_bo = xrt::bo(device, peak_bf16::AInputBytes,
                        xrt::bo::flags::normal, mm2s_a.group_id(0));
    auto b_bo = xrt::bo(device, peak_bf16::BInputBytes,
                        xrt::bo::flags::normal, mm2s_b.group_id(0));
    auto output_bo = xrt::bo(device, peak_bf16::COutputBytes,
                             xrt::bo::flags::normal, s2mm.group_id(0));

    auto* a_words = a_bo.map<std::uint32_t*>();
    auto* b_words = b_bo.map<std::uint32_t*>();
    auto* output = output_bo.map<std::uint32_t*>();

    fill_a(a_words);
    fill_b(b_words);
    for (int i = 0; i < peak_bf16::COutputWords; ++i) {
        output[i] = 0;
    }

    a_bo.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    b_bo.sync(XCL_BO_SYNC_BO_TO_DEVICE);
    output_bo.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    std::printf("BF16 local-buffer single-tile benchmark\n");
    std::printf(" - input GEMM shape       : %dx%dx%d\n",
                peak_bf16::InputM, peak_bf16::InputK, peak_bf16::InputN);
    std::printf(" - output C shape         : %dx%d\n",
                peak_bf16::InputM, peak_bf16::InputN);
    std::printf(" - MMUL shape             : %dx%dx%d bf16\n",
                peak_bf16::MmulM, peak_bf16::MmulK, peak_bf16::MmulN);
    std::printf(" - graph repeats          : 1\n");
    std::printf(" - MACs per repeat        : %d\n",
                peak_bf16::MacsPerRepeat);
    std::printf(" - A input words          : %d\n", peak_bf16::AInputWords);
    std::printf(" - B input words          : %d\n", peak_bf16::BInputWords);
    std::printf(" - C result words         : %d\n", peak_bf16::CResultWords);
    std::printf(" - C matrix buffer words  : %d\n", peak_bf16::CMatrixWords);
    std::printf(" - perf output words      : %d\n", peak_bf16::PerfOutputWords);
    std::printf(" - total output words     : %d\n", peak_bf16::COutputWords);
    std::fflush(stdout);

    auto a_run = mm2s_a(a_bo, nullptr, peak_bf16::AInputWords);
    auto b_run = mm2s_b(b_bo, nullptr, peak_bf16::BInputWords);
    auto out_run = s2mm(output_bo, nullptr, peak_bf16::COutputWords);

    const auto t0 = std::chrono::high_resolution_clock::now();
    graph.run(1);

    if (!wait_run("mm2s_a", a_run)) {
        graph.end();
        return 2;
    }
    if (!wait_run("mm2s_b", b_run)) {
        graph.end();
        return 3;
    }
    if (!wait_run("s2mm", out_run)) {
        graph.end();
        return 4;
    }

    graph.end();
    const auto t1 = std::chrono::high_resolution_clock::now();

    output_bo.sync(XCL_BO_SYNC_BO_FROM_DEVICE);

    const double seconds =
        std::chrono::duration<double>(t1 - t0).count();
    const std::uint64_t repeats = 1;
    const std::uint64_t macs_per_repeat = peak_bf16::MacsPerRepeat;
    const std::uint64_t total_macs = repeats * macs_per_repeat;
    const std::uint64_t total_flops = 2ull * total_macs;

    std::uint64_t checksum = 0;
    int nonzero_words = 0;
    for (int i = 0; i < peak_bf16::CResultWords; ++i) {
        checksum += output[i];
        if (output[i] != 0) {
            ++nonzero_words;
        }
    }

    const std::uint32_t* perf = output + peak_bf16::PerfWordOffset;
    const bool valid_perf =
        perf[0] == peak_bf16::PerfMagic && perf[1] == peak_bf16::PerfVersion;
    const std::uint64_t start_cycles =
        static_cast<std::uint64_t>(perf[2]) |
        (static_cast<std::uint64_t>(perf[3]) << 32);
    const std::uint64_t end_cycles =
        static_cast<std::uint64_t>(perf[4]) |
        (static_cast<std::uint64_t>(perf[5]) << 32);
    const std::uint64_t delta_cycles =
        static_cast<std::uint64_t>(perf[6]) |
        (static_cast<std::uint64_t>(perf[7]) << 32);
    const double aie_seconds =
        static_cast<double>(delta_cycles) /
        static_cast<double>(peak_bf16::AieClockHz);

    std::printf("AIE output C buffer\n");
    std::printf(" - result words checked   : %d\n", peak_bf16::CResultWords);
    std::printf(" - metadata word offset   : %d\n", peak_bf16::PerfWordOffset);
    std::printf(" - nonzero words          : %d\n", nonzero_words);
    std::printf(" - checksum_u64           : %llu\n",
                static_cast<unsigned long long>(checksum));
    std::printf(" - first words            : 0x%08x 0x%08x 0x%08x 0x%08x\n",
                output[0], output[1], output[2], output[3]);

    std::printf("AIE kernel cycle metadata\n");
    std::printf(" - valid                  : %s\n", valid_perf ? "yes" : "no");
    std::printf(" - magic/version          : 0x%08x / %u\n", perf[0], perf[1]);
    std::printf(" - start cycles           : %llu\n",
                static_cast<unsigned long long>(start_cycles));
    std::printf(" - end cycles             : %llu\n",
                static_cast<unsigned long long>(end_cycles));
    std::printf(" - delta cycles           : %llu\n",
                static_cast<unsigned long long>(delta_cycles));
    std::printf(" - assumed AIE clock Hz   : %llu\n",
                static_cast<unsigned long long>(peak_bf16::AieClockHz));
    if (valid_perf && delta_cycles != 0) {
        std::printf(" - AIE compute seconds    : %.9e\n", aie_seconds);
        std::printf(" - AIE-only GMAC/s        : %.3f\n",
                    static_cast<double>(total_macs) / aie_seconds / 1.0e9);
        std::printf(" - AIE-only GFLOP/s       : %.3f\n",
                    static_cast<double>(total_flops) / aie_seconds / 1.0e9);
    }

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

    if (nonzero_words == 0 || checksum == 0 || !valid_perf ||
        delta_cycles == 0) {
        std::printf("FAIL: invalid output C buffer\n");
        return 5;
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
