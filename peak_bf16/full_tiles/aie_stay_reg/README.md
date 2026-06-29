# BF16 Full-Tile Stay-Reg Peak Probe

This Test Harness target instantiates 304 AIE-ML kernels arranged as
8 rows by 38 columns. Each tile performs a register-resident BF16
`64x96x64` compute probe and records cycle metadata.

Metadata from the 304 tiles is gathered through 16 output PLIOs, with
19 tile records per PLIO. The host reports both per-tile average GFLOP/s
and full-array aggregate TFLOP/s using `max(end_cycles)-min(start_cycles)`.

Build and run:

```sh
cd /home/jnz/versal-aie/peak_mmul/peak_bf16/full_tiles/aie_stay_reg
make run GRAPH_ITERATIONS=1 RUN_PERF_MODE=0
```
