# INT8 single-tile register-resident probe

这个目录用于探测单个 AIE-ML tile 的 INT8 MMUL 计算吞吐上限。

特性：

- A/B 只从 input buffer 初始加载一次到寄存器
- 计时区间内反复使用寄存器里的 A/B 做 `MMUL::mac`
- 不追求 C 矩阵数学正确性
- 输出 buffer 尾部写入 cycle metadata
- 复用 AI-Engine-Test-Harness 的固定 PLIO/SD 镜像

默认逻辑计算量仍按 `64x96x64` GEMM 计：

```text
MACs = 64 * 96 * 64
OPS  = 2 * MACs
```

INT8 kernel 使用：

```cpp
aie::mmul<4, 8, 8, int8_t, int8_t>
```

单次 MMUL 是 `4 * 8 * 8 = 256 MAC`，对应 AIE-ML INT8 单 tile 理论吞吐
`256 MAC/cycle`。

## Build

```bash
cd /home/jnz/versal-aie/peak_mmul/peak_int8/single_tile/aie_stay_reg
make all
```

## Run

板卡上需要已经启动 Test Harness server，然后在开发机运行：

```bash
make run GRAPH_ITERATIONS=1 RUN_PERF_MODE=0
```

关注输出里的：

```text
AIE kernel cycle metadata
 - valid
 - delta cycles
 - AIE-only GOPS
 - AIE-only TOPS
```
