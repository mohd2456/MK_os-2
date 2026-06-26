#!/bin/sh
# ===================================================================================
# MK HYBRID OPERATING SYSTEM - ENVIRONMENT INITIALIZATION (`init.mk`)
# Target Baseline: 2010 MacBook Pro (POSIX/BSD Abstraction Layer)
# Scope: Isolated Partition Mounts & Real-Time Priority Allocation
# ===================================================================================

echo "=========================================================="
echo "Initializing MK OS Execution Layer... (Current Target: 2010 Baseline)"
echo "=========================================================="

# 1. ISOLATED DATASET PARTITIONING (Spec #17, #51)
# Decouple the heavy data models from the core system partitions
DATASET_MOUNT="/mnt/datasets"
mkdir -p "$DATASET_MOUNT"

# Target macOS disk layout variants gracefully for the 2010 baseline
if [ "$(uname)" = "Darwin" ]; then
    # Target external storage arrays or secondary bays on unibody hardware
    TARGET_DISK="/dev/disk1s1"
else
    TARGET_DISK="/dev/sda1"
fi

echo "[INIT] Mounting isolated model repository from $TARGET_DISK..."
mount -t ext4 "$TARGET_DISK" "$DATASET_MOUNT" 2>/dev/null || \
mount -t apfs "$TARGET_DISK" "$DATASET_MOUNT" 2>/dev/null

if [ $? -eq 0 ]; then
    echo "[SUCCESS] Dedicated dataset boundary established at $DATASET_MOUNT"
else
    echo "[WARN] Partition fallback active. Streaming restricted to root file storage matrix."
    export MK_STORAGE_FALLBACK=1
fi

# 2. RUNTIME RESOURCE & PRIVILEGE ELEVATION (Spec #21, #40)
# Lock pages into RAM to eliminate paging latency inside the tensor arenas
ulimit -l unlimited 2>/dev/null || echo "[INIT] Warning: Unable to bypass strict memlock limits."

# 3. EXECUTIVE EXPORT ENVIRONMENTS
export MK_MODE="boot"
export MK_DATASET_PATH="$DATASET_MOUNT"
export MK_CORE_CACHE_BUDGET=314572800  # Explicit 300MB cache ceiling (Spec #67)

# 4. TOKENIZED SCHEDULER ESCALATION (Spec #8, #40)
# Execute the native C++ binary with prioritized real-time execution tokens (nice -n -20)
if [ "$(id -u)" -eq 0 ]; then
    echo "[INIT] Elevating ai_core execution layer to real-time scheduling priority."
    exec nice -n -20 ./ai_core/mk_program --mode boot
else
    echo "[WARN] Non-root initialization. Running inside unprioritized timeslice windows."
    exec ./ai_core/mk_program --mode boot
fi