# BF16 全 Tile 寄存器驻留峰值性能测试

该 Test Harness 目标会实例化 304 个 AIE-ML kernel，并按照 8 行、38 列
映射到 AIE tile 阵列上。每个 tile 执行寄存器驻留的 BF16 计算测试，并记录
cycle 计数等元数据。

测试规模配置：

| 配置 | 每个 tile 的 GEMM 形状 |
|---|---|
| `tiny-size` | `64x96x64` |
| `medium-size` | `512x768x512` |
| `huge-size` | `4096x8192x4096` |

304 个 tile 的元数据通过 16 路输出 PLIO 汇总到 host，每路 PLIO 包含 19 个
tile 的记录。host 会基于平均 cycle 数和最慢 tile 的 cycle 数，分别统计单 tile
平均 GFLOP/s 以及整个 AIE 阵列的聚合 TFLOP/s。

编译和运行：

```sh
cd /home/jnz/versal-aie/peak_mmul/peak_bf16/full_tiles/aie_stay_reg_benchmark
make run BENCHMARK_PROFILE=tiny-size
make run BENCHMARK_PROFILE=medium-size
make run BENCHMARK_PROFILE=huge-size
```
