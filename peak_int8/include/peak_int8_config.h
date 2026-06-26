#pragma once

#include <cstddef>
#include <cstdint>

namespace peak_int8 {

// ── MMUL shape: M=4, K=8, N=8 ──────────────────────────────────────────────
// int8 has 2× the MAC density of bf16 on AIE-ML:
//   bf16:  mat<4×8, 8×4> → 128 MACs/mmul
//   int8:  mat<4×8, 8×8> → 256 MACs/mmul
// Each tile runs 2× mmul in parallel → 512 MACs/iter.
// At 312.5 MHz, theoretical peak per tile = 312.5M × 512 = 160 GOPS.
// VEK280 has 304 AIE-ML tiles × 160 GOPS ≈ 48.6 GOPS peak.
// GAMA reports 165 TOPS for int8 (85% efficiency) → total = ~194 TOPS.
static constexpr int M = 4;
static constexpr int K = 8;
static constexpr int N = 8;
static constexpr int MacsPerMmul   = M * K * N;           // 256
static constexpr int UnrollMacs     = MacsPerMmul;         // 256 (one full wave per iter)
static constexpr int IntOpsPerMac  = 2;                    // multiply + add

// ── Host ↔ AIE data movement ───────────────────────────────────────────────
static constexpr int InputWords  = 1;
static constexpr int OutputWords = 8;

// ── Graph / PLIO names ─────────────────────────────────────────────────────
static constexpr const char* GraphName   = "peak_int8_graph";
static constexpr const char* PlioInName  = "DataIn0";
static constexpr const char* PlioOutName = "DataOut0";

}  // namespace peak_int8
