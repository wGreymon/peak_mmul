# BF16 Single Tile Local-Buffer Peak

这个目录实现路线二：在板上真实运行 AIE tile，但核心性能只关注 AIE 从 `input_buffer` / local data memory 取数并执行 BF16 MMUL 的能力。

数据流：

```text
host 构造固定 A/B seed
  -> 两个 mm2s PL kernel
  -> AIE input_buffer<bfloat16>
  -> AIE local data memory load_v
  -> vector register
  -> aie::mmul<8,8,4,bfloat16,bfloat16,accauto>
  -> accumulator register
  -> 少量统计结果输出
```

这个 benchmark 不测完整 DDR/PLIO/NoC 端到端带宽。输入只用于初始化 AIE local buffer；AIE kernel 内部用很大的 repeat 重复计算，摊薄输入和启动开销。

## 三个实验层级

```text
aie_unconstr_buf:
  不约束 kernel/buffer placement，让编译器自动决定。

aie_conf_buf:
  约束 input buffer 放在 kernel 本地 local data memory，
  但具体 bank/address 交给编译器。

aie_constr_buf:
  固定 kernel tile 和 input buffer 双缓冲地址，
  用于进一步观察手动 bank/address 约束对性能的影响。
```

当前建议先跑 `aie_conf_buf`，它最贴近我们主线假设：A/B seed 已经进入 AIE local data memory，AIE 反复从同一片 local buffer 取数计算。

## 构建

以 `aie_conf_buf` 为例：

```bash
cd /home/jnz/versal-aie/peak_mmul/peak_bf16/single_tile/aie_conf_buf
make all
```

其他两个版本：

```bash
cd /home/jnz/versal-aie/peak_mmul/peak_bf16/single_tile/aie_unconstr_buf
make all

cd /home/jnz/versal-aie/peak_mmul/peak_bf16/single_tile/aie_constr_buf
make all
```

## 上板运行

以 `aie_conf_buf` 为例：

```bash
cd /home/jnz/versal-aie/peak_mmul/peak_bf16/single_tile/aie_conf_buf

scp build/peak_bf16_single_tile_host \
    build/peak_bf16_single_tile_link.xclbin \
    petalinux@192.168.2.57:/home/petalinux/stx_kernels_test/
```

板上执行：

```bash
cd /home/petalinux/stx_kernels_test
chmod +x peak_bf16_single_tile_host
./peak_bf16_single_tile_host peak_bf16_single_tile_link.xclbin
```

注意：host 程序需要通过 XRT 打开 `mm2s_1`、`mm2s_2`、`s2mm_1` 三个 PL compute unit，因此请使用 `peak_bf16_single_tile_link.xclbin`。Makefile 也会生成 `peak_bf16_single_tile_package.xclbin`，它是 package 阶段产物，主要保留 AIE 加载相关信息；直接传给当前 host 会出现 `No compute units matching 'mm2s:{mm2s_1}'`。

## 计算量

当前第一版只计算固定的 16x8 C 输出区域，每个 repeat 的有效计算量为：

```text
4 * (8 * 8 * 4) * (96 / 8) = 12288 MAC
```

总 MAC：

```text
DefaultRepeats * 12288
```

按 FLOP 统计时使用：

```text
1 MAC = 2 FLOPs
```
