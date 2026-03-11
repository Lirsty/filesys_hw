#!/bin/bash

# Configuration
FILE_NAME="100MB.bin"
FILE_SIZE_MB=100
SOURCES=("HW111.c" "HW112.c" "HW113.c")
COOL_DOWN_TIME=10 # Seconds to wait between tests

# Check for root privileges (required to drop caches and run fstrim)
if [ "$EUID" -ne 0 ]; then
  echo "Error: Please run this script with sudo."
  exit 1
fi

echo "=========================================================="
echo "    File System I/O Benchmark Automation (Hardware Aware)"
echo "=========================================================="

for SRC in "${SOURCES[@]}"; do
    PROG="${SRC%.c}" # Extract program name (e.g., HW111)

    echo ""
    echo ">>> Testing Module: $PROG"

    # 1. Compilation
    echo "   [1/5] Compiling $SRC..."
    gcc -o "$PROG" "$SRC"
    if [ $? -ne 0 ]; then
        echo "   Error: Compilation of $SRC failed. Skipping..."
        continue
    fi

    # 2. Data Preparation
    # Using /dev/urandom to bypass filesystem-level compression/deduplication
    echo "   [2/5] Generating $FILE_SIZE_MB MB test file with dd..."
    dd if=/dev/urandom of=$FILE_NAME bs=1M count=$FILE_SIZE_MB status=none

    # 3. Environment Preparation (The "Cold Start" Protocol)
    # This ensures neither the OS nor the SSD Hardware buffers affect the result
    echo "   [3/5] Cleaning caches and trimming SSD..."
    sync                         # Flush dirty pages from RAM to Disk
    fstrim -v /                  # Inform SSD about deleted blocks to allow Background GC
    echo 3 > /proc/sys/vm/drop_caches # Clear OS Page Cache, dentries, and inodes

    # 4. Execution
    echo "   [4/5] Executing Benchmark..."
    echo "----------------------------------------------------------"
    ./"$PROG"
    echo "----------------------------------------------------------"

    # 5. Cleanup and Hardware Cool-down
    echo "   [5/5] Cleaning up and cooling down..."
    rm "$PROG"

    # Wait to allow SSD SLC Cache to flush and controller to stabilize
    if [ "$SRC" != "${SOURCES[-1]}" ]; then
        echo "   Waiting $COOL_DOWN_TIME seconds for SSD hardware recovery..."
        sleep $COOL_DOWN_TIME
    fi
done

# Final Cleanup of the test binary and data
if [ -f "$FILE_NAME" ]; then
    rm "$FILE_NAME"
fi

echo ""
echo "All benchmarks completed successfully."
echo "=========================================================="
