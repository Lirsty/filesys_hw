#!/bin/bash

# Configuration
FILE_NAME="100MB.bin"
FILE_SIZE_MB=100
PROGS=("HW112" "HW113") 
COOL_DOWN_TIME=10

# Check for root privileges
if [ "$EUID" -ne 0 ]; then
  echo "Error: Please run this script with sudo."
  exit 1
fi

echo "=========================================================="
echo "    Comparative Perf Profiling: HW112 vs HW113"
echo "=========================================================="

for PROG in "${PROGS[@]}"; do
    SRC="${PROG}.c"
    
    if [ ! -f "$SRC" ]; then
        echo "   Error: $SRC not found, skipping..."
        continue
    fi

    echo ">>> Starting profiling for $PROG..."

    # 1. Compilation
    echo "   [1/5] Compiling $SRC with debug symbols..."
    gcc -g -o "$PROG" "$SRC"
    if [ $? -ne 0 ]; then
        echo "   Error: Compilation of $SRC failed."
        continue
    fi

    # 2. Data Preparation
    echo "   [2/5] Generating $FILE_SIZE_MB MB test file..."
    dd if=/dev/urandom of=$FILE_NAME bs=1M count=$FILE_SIZE_MB status=none

    # 3. Environment Preparation
    echo "   [3/5] Cleaning environment (drop_caches & sync)..."
    sync
    echo 3 > /proc/sys/vm/drop_caches
    sleep $COOL_DOWN_TIME

    # 4. Execution with Perf
    PERF_DATA="perf_${PROG}.data"
    echo "   [4/5] Recording call-graph data into $PERF_DATA..."
    # perf record -g -o "$PERF_DATA" ./"$PROG"
    ./"$PROG"

    # 5. Cleanup for next run
    echo "   [5/5] Cleaning up $PROG files..."
    [ -f "$PROG" ] && rm "$PROG"
    [ -f "$FILE_NAME" ] && rm "$FILE_NAME"
    
    echo "   Done with $PROG."
    echo "----------------------------------------------------------"
done

echo ""
echo "Profiling completed successfully!"
echo "To compare the results, run these in two terminals:"
echo "1. sudo perf report -i perf_HW112.data"
echo "2. sudo perf report -i perf_HW113.data"
echo "=========================================================="