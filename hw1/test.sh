#!/bin/bash

# Configuration
FILE_NAME="100MB.bin"
FILE_SIZE_MB=100
SOURCES=("HW111.c" "HW112.c" "HW113.c")

# Check for root privileges (required to drop caches)
if [ "$EUID" -ne 0 ]; then
  echo "Error: Please run this script with sudo."
  exit 1
fi

echo "=========================================================="
echo "   File System I/O Benchmark Automation"
echo "=========================================================="

for SRC in "${SOURCES[@]}"; do
    PROG="${SRC%.c}" # Extract program name (e.g., HW111)
    
    echo ""
    echo ">>> Testing Module: $PROG"
    
    # 1. Compilation
    echo "   [1/4] Compiling $SRC..."
    gcc -o "$PROG" "$SRC"
    if [ $? -ne 0 ]; then
        echo "   Error: Compilation of $SRC failed. Skipping..."
        continue
    fi

    # 2. Data Preparation (dd)
    # Using /dev/urandom to prevent filesystem-level compression optimizations
    echo "   [2/4] Generating $FILE_SIZE_MB MB test file with dd..."
    dd if=/dev/urandom of=$FILE_NAME bs=1M count=$FILE_SIZE_MB status=none
    
    # 3. Clear Page Cache (Crucial for Cold Start measurement)
    echo "   [3/4] Flushing and dropping Page Cache..."
    sync # Flush dirty pages to disk first
    echo 3 > /proc/sys/vm/drop_caches
    
    # 4. Execution
    echo "   [4/4] Executing Benchmark..."
    echo "----------------------------------------------------------"
    ./"$PROG"
    echo "----------------------------------------------------------"
    
    # Cleanup binaries to keep the directory clean
    rm "$PROG"
done

# Final Cleanup of the test data
if [ -f "$FILE_NAME" ]; then
    rm "$FILE_NAME"
fi

echo ""
echo "All benchmarks completed successfully."
echo "=========================================================="