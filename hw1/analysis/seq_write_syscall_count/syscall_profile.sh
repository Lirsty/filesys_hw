#!/bin/bash

# Configuration
FILE_NAME="100MB.bin"
FILE_SIZE_MB=100
SOURCES=("HW111.c" "HW112.c") 
COOL_DOWN_TIME=10 

if [ "$EUID" -ne 0 ]; then
  echo "Error: Please run this script with sudo."
  exit 1
fi

echo "=========================================================="
echo "    Syscall Profiling: C Library vs. System Calls"
echo "=========================================================="

for SRC in "${SOURCES[@]}"; do
    PROG="${SRC%.c}"
    LOG_FILE="result_${PROG}.txt" # Save results to a permanent file

    echo ""
    echo ">>> Testing Module: $PROG"

    # 1. Compilation
    echo "   [1/5] Compiling $SRC..."
    gcc -o "$PROG" "$SRC"
    if [ $? -ne 0 ]; then
        echo "   Error: Compilation of $SRC failed."
        continue
    fi

    # 2. Data Preparation
    echo "   [2/5] Generating $FILE_SIZE_MB MB test file ($FILE_NAME)..."
    dd if=/dev/urandom of=$FILE_NAME bs=1M count=$FILE_SIZE_MB status=none

    # 3. Environment Preparation
    echo "   [3/5] Cleaning caches and trimming SSD..."
    sync
    fstrim -v / 2>/dev/null
    echo 3 > /proc/sys/vm/drop_caches

    # 4. Execution & Profiling
    echo "   [4/5] Profiling System Calls via strace..."
    echo "----------------------------------------------------------"
    # Run strace and save the FULL output to a file, then cat it to screen
    strace -c ./"$PROG" > /dev/null 2> "$LOG_FILE"
    cat "$LOG_FILE" # Print the full table so you can see it
    echo "----------------------------------------------------------"
    echo "   Full log saved to: $LOG_FILE"

    # 5. Cleanup and Hardware Cool-down
    echo "   [5/5] Cleaning up and cooling down..."
    rm "$PROG"

    if [ "$SRC" != "${SOURCES[-1]}" ]; then
        echo "   Waiting $COOL_DOWN_TIME seconds for hardware recovery..."
        sleep $COOL_DOWN_TIME
    fi
done

if [ -f "$FILE_NAME" ]; then
    rm "$FILE_NAME"
fi

echo ""
echo "Profiling completed. Check result_HW111.txt and result_HW112.txt."
echo "=========================================================="