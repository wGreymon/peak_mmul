#!/bin/bash
#
# build_and_deploy_int8.sh
# 编译 INT8 峰值 benchmark 并 SCP 上板
#

set -e
BOARD_IP="192.168.2.57"
BOARD_DIR="/mnt/sd-mmcblk0p1"
ITERATIONS=${1:-10000000}

PROJ_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${PROJ_DIR}/build"
XCLBIN="${BUILD_DIR}/peak_int8.xclbin"
HOST="${BUILD_DIR}/peak_int8_host"

echo "============================================================"
echo "  INT8 Peak GEMM — Build & Deploy to VEK280"
echo "============================================================"

# 1. Build
echo ""
echo "[1/3] Building INT8 peak benchmark..."
cd "${PROJ_DIR}"
make clean > /dev/null 2>&1 || true
make all

if [[ ! -f "$XCLBIN" ]]; then
    echo "ERROR: build failed, xclbin not found at $XCLBIN"
    exit 1
fi
echo "Build OK: $(du -sh $XCLBIN | cut -f1) xclbin, $(du -sh $HOST | cut -f1) host"

# 2. SCP deploy
echo ""
echo "[2/3] Deploying to $BOARD_IP..."
scp -o StrictHostKeyChecking=no \
    "$XCLBIN" \
    "$HOST"  \
    "root@${BOARD_IP}:${BOARD_DIR}/"

# 3. Run
echo ""
echo "[3/3] Running on board ($ITERATIONS iterations)..."
echo "============================================================"
ssh -o StrictHostKeyChecking=no "root@${BOARD_IP}" << 'ENDSSH'
set -e
cd /mnt/sd-mmcblk0p1
chmod +x peak_int8_host
./peak_int8_host peak_int8.xclbin "$1"
ENDSSH

echo ""
echo "Done."
