# peak_mmul

Versal AIE-ML 上的 gemm 峰值计算性能实验工程。

## 目录结构

- `single_tile/`：单个 AIE tile 的峰值 micro-kernel 实验。
- `multi_tiles/`：多个 AIE tile 协同计算实验，包括 cascade/reduction 原型。
- `full_tiles/`：尽可能使用更多 AIE tile （尽量用满）的大规模峰值实验。
- `best_G_exp/`：探索 reduction chain 长度 `G` 的取值。
- `swizzling_pos/`：探索 AIE pack 的错位放置和布局策略。
- `common/`：共享的 host、PL、配置、脚本和性能统计工具。