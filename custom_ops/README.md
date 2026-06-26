# Peak MMUL

Standalone VEK280 AIE-ML compute microbenchmark. This project does not use
AI-Engine-Test-Harness or its client/server flow.

The host executable loads an xclbin through XRT, sends one runtime parameter
(`repeats`) through a tiny `mm2s` PL kernel, runs one AIE graph iteration, and
collects a small checksum through `s2mm`.

The AIE kernel keeps data local and repeatedly executes:

```text
aie::mmul<4,8,8,bfloat16,bfloat16>::mac(...)
```

This is intended to measure a compute-bound single-tile peak estimate, not
DDR/PLIO bandwidth.

## Build

```bash
cd /home/jnz/versal-aie/peak_mmul
make all
```

Important outputs:

```text
build/peak_mmul_link.xclbin
build/peak_mmul_host
```

`build/peak_mmul_link.xclbin` is the full link-stage xclbin with the PL
bitstream, AIE metadata, kernel layout, and kernel base addresses. Use this
file for the standalone XRT host run.

## Run On VEK280

Copy both files to the board:

```bash
scp build/peak_mmul_link.xclbin build/peak_mmul_host root@<board-ip>:/run/media/mmcblk0p1/
```

On the board:

```bash
cd /run/media/mmcblk0p1
export XILINX_XRT=/usr
export LD_LIBRARY_PATH=/usr/lib:$LD_LIBRARY_PATH
chmod +x peak_mmul_host
./peak_mmul_host peak_mmul_link.xclbin 1000000
```

The second argument is the AIE repeat count. Increase it if the reported time is
too short or unstable.

## Metric

For the current kernel:

```text
MACs per MMUL = 4 * 8 * 8 = 256
FLOPs per MMUL = 512
MMULs per repeat = 64
```

The host reports:

```text
total MACs = repeats * 64 * 256
GFLOP/s    = total MACs * 2 / seconds / 1e9
```

This first version measures one AIE tile. Multi-tile replication can be added
after the single-tile number is stable.
