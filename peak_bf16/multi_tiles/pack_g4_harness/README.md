# BF16 single-tile Test Harness 路线

这个目录用于在当前 Test Harness SD 镜像上安全验证 BF16 single-tile GEMM kernel。

这条路线只编译 AIE graph，并将它 package 到 `vek280_test_harness.xsa` 对应的固定 PL DMA/test harness 硬件上：

- 不编译自定义 `mm2s/s2mm` PL kernel
- 不执行 `v++ link` 生成新的完整硬件 bitstream
- 不使用 `peak_bf16_single_tile_link.xclbin`
- host 通过 Test Harness server 发送由 host 构造的 BF16 数据

## 构建

```bash
cd /home/jnz/versal-aie/peak_mmul/peak_bf16/single_tile/aie_conf_buf_harness
make all
```

产物目录：

```text
build/pkg.hw.xilinx_vek280_base_202520_1.xpfm/
```

其中包含：

- `client_exe`
- `vek280_test_harness.xclbin`
- `run_script.sh`

默认 `client_exe` 是开发机 x86 程序，用于连接板卡上的 Test Harness server：

```bash
make run GRAPH_ITERATIONS=1
```

## 上传到板卡运行

如果希望把 `client_exe` 拷到板上本地运行，先用 ARM 交叉编译：

```bash
make cleanall
make all CPU=arm
```

然后上传：

```bash
scp -r build/pkg.hw.xilinx_vek280_base_202520_1.xpfm/* \
    petalinux@192.168.2.57:/home/petalinux/stx_kernels_test/
```

## 板卡运行

```bash
cd /home/petalinux/stx_kernels_test
chmod +x run_script.sh client_exe
LD_LIBRARY_PATH=/opt/xilinx/xrt/lib ./run_script.sh
```

性能模式：

```bash
LD_LIBRARY_PATH=/opt/xilinx/xrt/lib RUN_PERF_MODE=1 PERF_REPETITIONS=1 ./run_script.sh
```
