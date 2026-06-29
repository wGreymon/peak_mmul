#!/bin/sh

if [ -z "$SERVER_IP_PORT" ]; then
    export SERVER_IP_PORT=192.168.2.57:8080
fi

export GRAPH_ITERATIONS=${GRAPH_ITERATIONS:-1}
export NUM_REPETITIONS=${NUM_REPETITIONS:-1}
export CHANNEL_DELAY=${CHANNEL_DELAY:-0}
export RUN_PERF_MODE=${RUN_PERF_MODE:-0}
export PERF_REPETITIONS=${PERF_REPETITIONS:-1}

./client_exe "$GRAPH_ITERATIONS"
return_code=$?
if [ $return_code -ne 0 ]; then
    echo "ERROR: PEAK BF16 HARNESS RUN FAILED, RC=$return_code"
    exit $return_code
else
    echo "INFO: PEAK BF16 HARNESS RUN COMPLETED, RC=0"
fi
exit $return_code
