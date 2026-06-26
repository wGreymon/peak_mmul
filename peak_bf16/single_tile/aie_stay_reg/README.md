# BF16 single-tile register-resident probe

这个目录用于探测单个 AIE tile 的 BF16 MMUL 计算吞吐上限。

和 `aie_conf_buf_harness` 的区别：

- A/B 只从 input buffer 初始加载到寄存器一次
- 计时区间内反复使用寄存器里的 A/B 做 `MMUL::mac`
- 不追求 C 矩阵数学正确性
- 只依赖输出 buffer 尾部的 cycle metadata 统计 AIE 内部 cycles
- 仍然复用 AI-Engine-Test-Harness 的固定 PLIO/SD 镜像

默认逻辑计算量仍按 `64x96x64` GEMM 计：

```text
MACs  = 64 * 96 * 64
FLOPs = 2 * MACs
```

## Build

```bash
cd /home/jnz/versal-aie/peak_mmul/peak_bf16/single_tile/aie_stay_reg
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
 - AIE-only GFLOP/s
```

`AIE host end-to-end latency` 包含 host、socket、test harness 调度和传输开销，不代表纯 AIE 计算时间。
