#!/bin/bash
#
# Run script for VEK280 INT8 Peak GEMM benchmark
#

export SERVER_IP_PORT=${SERVER_IP_PORT:-"192.168.2.57:8080"}
export SERVER_URL="http://${SERVER_IP_PORT}"

BUILD_DIR="$(cd "$(dirname "$0")/build" && pwd)"
XCLBIN_PATH="${BUILD_DIR}/peak_int8.xclbin"
HOST_PATH="${BUILD_DIR}/peak_int8_host"

if [[ ! -f "$XCLBIN_PATH" ]]; then
    echo "ERROR: xclbin not found at $XCLBIN_PATH"
    echo "Run 'make -C peak_int8 all' first."
    exit 1
fi

if [[ ! -f "$HOST_PATH" ]]; then
    echo "ERROR: host not found at $HOST_PATH"
    echo "Run 'make -C peak_int8 host' first."
    exit 1
fi

# Check connectivity
echo "Checking stxklib-server at $SERVER_URL..."
if ! curl -s --connect-timeout 5 "$SERVER_URL/status" > /dev/null 2>&1; then
    echo "WARNING: stxklib-server unreachable. Will attempt direct execution."
fi

# Send xclbin and host to board
echo "Deploying to board..."
curl -s -T "$XCLBIN_PATH" "$SERVER_URL/upload/peak_int8.xclbin"
curl -s -T "$HOST_PATH" "$SERVER_URL/upload/peak_int8_host"

# Run benchmark on board
echo ""
echo "Launching INT8 peak benchmark (10M iterations)..."
echo "================================================="
curl -s -X POST "$SERVER_URL/run" \
    -d "{\"exec\": \"$HOST_PATH\", \"args\": [\"/mnt/sd-mmcblk0p1/peak_int8.xclbin\", \"10000000\"]}" \
    | python3 -m json.tool

echo ""
echo "Benchmark complete."
