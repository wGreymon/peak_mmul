# VEK280 / VE2802 AIE-ML Peak Performance Report

本文档用于维护 `custom_ops` 算子工程的理论峰值分析。后续实测数据、
kernel 版本变化、多 tile 扩展结果也可以继续追加到这里。

## 1. 计算目标

当前关注的是 VEK280 板卡上 VE2802 AIE-ML array 的理论计算峰值，尤其是：

- 官方器件最大规格下的 INT8 peak
- 当前 VEK280 base platform 下更适合作为实验基准的 INT8 peak
- 当前单 tile `peak_mmul` microbenchmark 的理论上限

通用公式：

```text
Peak OPS/s = number_of_tiles
           * AIE_frequency
           * MACs_per_cycle_per_tile
           * ops_per_MAC
```

其中通常按 AI 算力统计习惯：

```text
1 MAC = 1 multiply + 1 add = 2 operations
```

所以：

```text
Peak TOPS = tiles * freq_GHz * MACs_per_cycle_per_tile * 2 / 1000
```

## 2. VE2802 AIE-ML Tile 数量

VE2802 器件的 AIE-ML tile 数量为：

```text
304 AIE-ML tiles
```

也可以从阵列规模理解为：

```text
38 columns * 8 AIE-ML compute rows = 304 tiles
```

本机 VEK280 base platform 的 AIE 描述文件也能看到 38 个 column，以及 core row
配置：

```text
/tools/Xilinx/2025.2/Vitis/base_platforms/xilinx_vek280_base_202520_1/hw/sdt/pl.dtsi

xlnx,aie-col-dict = "col00 ... col37";
xlnx,core-rows = /bits/ 8 <3 8>;
xlnx,aie-site = "AIE_ML";
```

其中 `<3 8>` 可以理解为 AIE-ML core row 从 row 3 开始，共 8 行。

## 3. INT8 单 Tile 理论吞吐

AMD AIE-ML 架构手册中给出的 INT8 matrix multiply 峰值为：

```text
int8 x int8: 256 MAC/cycle/tile
```

按 `1 MAC = 2 ops` 统计：

```text
256 MAC/cycle/tile * 2 ops/MAC = 512 ops/cycle/tile
```

因此 INT8 峰值的核心参数是：

```text
MACs_per_cycle_per_tile = 256
ops_per_MAC             = 2
ops_per_cycle_per_tile  = 512
```

## 4. 官方最大规格 Peak

AMD datasheet 的 AIE array 最大频率表中，`-2 @ 0.88V(H)` 条件下：

```text
F_MAX = 1300 MHz
```

用这个频率计算 VE2802 官方最大 INT8 理论峰值：

```text
304 tiles * 1.3 GHz * 256 MAC/cycle/tile * 2 ops/MAC
= 202.3424 TOPS
```

因此官方产品表里的：

```text
AI Engine Peak Perf INT8 = 202 TOPS
```

可以理解为按下面这组参数计算：

```text
tiles      = 304
frequency  = 1.3 GHz
INT8 MMUL  = 256 MAC/cycle/tile
MAC metric = 2 ops/MAC
```

这个值是器件数据手册条件下的理论最大峰值，不代表当前板卡工程一定运行在
1.3 GHz。

## 5. VEK280 当前 Platform Peak

当前工程使用的 VEK280 base platform 是：

```text
/tools/Xilinx/2025.2/Vitis/base_platforms/xilinx_vek280_base_202520_1/xilinx_vek280_base_202520_1.xpfm
```

该 platform 的设备树描述中明确给出了 AIE 频率：

```text
/tools/Xilinx/2025.2/Vitis/base_platforms/xilinx_vek280_base_202520_1/hw/sdt/pl.dtsi

xlnx,aie-max-freq = <1250>;
xlnx,aie-core-ref-ctrl-freqmhz = <1250>;
clock-frequency = <1250000000>;
```

因此，对当前 VEK280 实验工程，更合理的 INT8 理论基准是：

```text
304 tiles * 1.25 GHz * 256 MAC/cycle/tile * 2 ops/MAC
= 194.56 TOPS
```

后续如果报告当前板卡实测达成率，建议使用：

```text
INT8 platform peak = 194.56 TOPS
```

而不是官方最大规格的 `202 TOPS`。

## 6. 其他精度的理论峰值

同样使用 VEK280 platform 的 `1.25 GHz`，可以得到：

```text
INT8    : 304 * 1.25 * 256 * 2 / 1000 = 194.56 TOPS
INT8x16 : 304 * 1.25 * 128 * 2 / 1000 =  97.28 TOPS
INT16   : 304 * 1.25 *  64 * 2 / 1000 =  48.64 TOPS
BF16    : 304 * 1.25 * 128 * 2 / 1000 =  97.28 TFLOPS
```

如果使用 datasheet 最大 `1.3 GHz`，则为：

```text
INT8    : 304 * 1.30 * 256 * 2 / 1000 = 202.34 TOPS
INT8x16 : 304 * 1.30 * 128 * 2 / 1000 = 101.17 TOPS
INT16   : 304 * 1.30 *  64 * 2 / 1000 =  50.59 TOPS
BF16    : 304 * 1.30 * 128 * 2 / 1000 = 101.17 TFLOPS
```

注意：BF16 这里使用 `128 MAC/cycle/tile` 的理论 matrix multiply 吞吐，
单位应写作 `TFLOPS`，而不是 `TOPS`。

## 7. 当前 custom_ops 单 Tile BF16 Benchmark

当前 `custom_ops` 工程里的 AIE kernel 是一个单 tile BF16 MMUL microbenchmark。
核心 MMUL 形状为：

```cpp
aie::mmul<4, 8, 8, bfloat16, bfloat16>
```

单次 MMUL MAC 数：

```text
4 * 8 * 8 = 256 MAC
```

当前 kernel 每个 repeat 执行：

```text
64 MMUL MAC operations
```

因此 host 中的计算量统计为：

```text
MACs = repeats * 64 * 256
FLOPs = MACs * 2
GFLOP/s = FLOPs / seconds / 1e9
```

需要注意：这个 benchmark 目前只使用 1 个 AIE tile，所以它不能直接代表
VE2802 全阵列峰值。它更适合先验证单 tile kernel 是否足够 compute-bound。

如果按 VEK280 platform 的 `1.25 GHz` 估算单 tile BF16 理论峰值：

```text
single_tile_BF16_peak = 1.25 GHz * 128 MAC/cycle * 2 ops/MAC
                      = 320 GFLOP/s
```

当前代码中单次 `aie::mmul<4,8,8,bfloat16,bfloat16>` 包含 256 MAC，但架构
BF16 稳态吞吐通常按 `128 MAC/cycle/tile` 统计，因此需要通过 compiler schedule
和实测 cycle 判断 kernel 是否接近理论吞吐。

## 8. 建议的报告口径

后续写实验报告时建议统一使用下面两种口径：

```text
Official device maximum:
VE2802 INT8 peak = 202 TOPS
Condition: 304 tiles, 1.3 GHz, 256 INT8 MAC/cycle/tile

This VEK280 platform:
VE2802 INT8 peak = 194.56 TOPS
Condition: 304 tiles, 1.25 GHz, 256 INT8 MAC/cycle/tile
```

对于公司内部算子验证，建议主要使用第二个值计算达成率：

```text
achieved_ratio = measured_TOPS / 194.56 TOPS
```

这样更能反映当前 `xilinx_vek280_base_202520_1` platform 下的真实实验上限。

## 9. 参考来源

- AMD DS958, Versal AI Edge Series Data Sheet: AIE array `F_MAX`
- AMD AM020, Versal Adaptive SoC AIE-ML Architecture Manual: AIE-ML matrix
  multiply MAC throughput
- AMD VEK280 base platform local metadata:
  `/tools/Xilinx/2025.2/Vitis/base_platforms/xilinx_vek280_base_202520_1/hw/sdt/pl.dtsi`
- Local project:
  `peak_mmul/custom_ops/aie/peak_mmul_kernel.cc`
