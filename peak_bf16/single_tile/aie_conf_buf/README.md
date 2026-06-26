# BF16 single-tile standalone 路线

这个目录用于构建不依赖 Test Harness 的 BF16 single-tile 上板镜像。

目标是让下面这些东西来自同一套硬件设计：

- AIE graph/kernel
- PL `mm2s/s2mm` kernel
- `system.cfg` 中的 PLIO 连接
- standalone XSA
- runtime xclbin
- SD card image
- aarch64 host 程序

不要在 Test Harness SD 镜像上运行这里生成的完整硬件 xclbin。

这里的 runtime xclbin 来自同一次 `v++ -l` 生成的完整硬件 xclbin，包含 `mm2s/s2mm` CU 地址。它只允许放进本目录 package 出来的 standalone SD 镜像中使用。

## 构建目标

```bash
make aie       # 生成 build/libadf.a
make kernels   # 生成 build/mm2s.xo 和 build/s2mm.xo
make xsa       # link AIE + PL，生成 standalone XSA
make xclbin    # 拷贝同源 full hardware xclbin 作为 runtime xclbin
make host      # 生成 aarch64 host
make sd_card   # 打包 SD card 镜像
make all       # 等价于 make sd_card
```

最终产物：

```text
build/package_standalone/sd_card.img
build/package_standalone/sd_card.img.zip
```

## 构建命令

```bash
cd /home/jnz/versal-aie/peak_mmul/peak_bf16/single_tile/aie_conf_buf
make sd_card
```

默认使用：

```text
/tools/Xilinx/2025.2/Vitis/base_platforms/xilinx_vek280_base_202520_1/xilinx_vek280_base_202520_1.xpfm
/opt/xilinx-versal-common-v2025.2/rootfs.ext4
/opt/xilinx-versal-common-v2025.2/Image
```

## 烧录前检查

构建完成后先检查产物：

```bash
ls -lh build/package_standalone/sd_card.img
ls -lh build/peak_bf16_single_tile_standalone.xsa
ls -lh build/peak_bf16_single_tile_standalone.xclbin
ls -lh build/peak_bf16_single_tile_host
```

确认 runtime xclbin 里有 PL kernel 地址信息：

```bash
xclbinutil --info --input build/peak_bf16_single_tile_standalone.xclbin | less
```

应该能看到：

```text
Kernels: s2mm, mm2s
Instance: mm2s_1
Instance: mm2s_2
Instance: s2mm_1
Base Address: ...
```

如果只看到 `Action Mask(s): LOAD_AIE`，并且 kernel base address 是 `--`，不要烧录。

注意：这个 xclbin 是完整硬件 xclbin。它不能在 Test Harness 镜像上运行，只能随本目录生成的 `sd_card.img` 一起使用。

## 烧录后运行

镜像里会放入：

```text
peak_bf16_single_tile_host
peak_bf16_single_tile_standalone.xclbin
run_standalone.sh
```

板卡启动后先查找脚本位置：

```bash
find / -name run_standalone.sh 2>/dev/null
```

然后进入脚本所在目录执行：

```bash
cd <run_standalone.sh 所在目录>
./run_standalone.sh
```

## 风险边界

这条路线会加载自己的完整 PL+AIE 设计，只应运行在用本目录 XSA/package 出来的 SD 镜像上。

不要把这里的 xclbin 拷到 Test Harness 镜像上运行。
