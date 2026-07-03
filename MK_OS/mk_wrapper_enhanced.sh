#!/bin/bash
# ============================================================
# MK OS - Enhanced Startup Wrapper
# Checks system resources before starting MK
# Sets up cgroups if available
# Sources configuration
# Starts background monitors
# Implements graceful shutdown on signals
# ============================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MK_BINARY="${SCRIPT_DIR}/mk_os"
CONFIG_DIR="${HOME}/.mk_os"
CONFIG_FILE="${CONFIG_DIR}/config.json"
PID_FILE="${CONFIG_DIR}/mk_os.pid"
LOG_FILE="${CONFIG_DIR}/mk_os.log"
MONITOR_PID=""

# ============================================================
# Resource Check - Don't start if system can't handle it
# ============================================================
check_resources() {
    echo "[MK] Checking system resources..."

    # Check available RAM (at least 512MB free)
    if command -v free >/dev/null 2>&1; then
        local free_mb
        free_mb=$(free -m 2>/dev/null | awk '/^Mem:/ {print $7}' || echo "0")
        if [ "${free_mb:-0}" -lt 512 ]; then
            echo "[MK] WARNING: Only ${free_mb}MB RAM available (minimum: 512MB)"
            echo "[MK] System may be unstable. Proceeding with caution."
        else
            echo "[MK] RAM OK: ${free_mb}MB available"
        fi
    elif command -v vm_stat >/dev/null 2>&1; then
        # macOS
        local pages_free
        pages_free=$(vm_stat 2>/dev/null | grep "Pages free" | awk '{print $3}' | tr -d '.')
        local free_mb_mac=$(( (pages_free * 4096) / (1024 * 1024) ))
        if [ "${free_mb_mac:-0}" -lt 256 ]; then
            echo "[MK] WARNING: Low free RAM on macOS"
        else
            echo "[MK] RAM OK: ~${free_mb_mac}MB free pages"
        fi
    fi

    # Check CPU load (don't start if load > num_cores * 2)
    local cores
    cores=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo "4")
    local max_load
    max_load=$((cores * 2))
    local current_load
    current_load=$(uptime | awk -F'load average:' '{print $2}' | awk -F',' '{print $1}' | tr -d ' ')
    if [ -n "${current_load}" ]; then
        local load_int
        load_int=$(echo "${current_load}" | awk '{printf "%d", $1}')
        if [ "${load_int}" -gt "${max_load}" ]; then
            echo "[MK] WARNING: High system load (${current_load}). Max recommended: ${max_load}"
            echo "[MK] Waiting 10 seconds for load to settle..."
            sleep 10
        else
            echo "[MK] CPU load OK: ${current_load} (max: ${max_load})"
        fi
    fi

    # Check disk space (at least 1GB free)
    local free_disk_kb
    free_disk_kb=$(df "${SCRIPT_DIR}" 2>/dev/null | tail -1 | awk '{print $4}')
    if [ "${free_disk_kb:-0}" -lt 1048576 ]; then
        echo "[MK] WARNING: Low disk space. Less than 1GB free."
    else
        echo "[MK] Disk space OK"
    fi

    echo "[MK] Resource check complete."
}

# ============================================================
# Cgroup Setup (Linux only, optional)
# ============================================================
setup_cgroups() {
    # Only attempt on Linux with cgroup v2 support
    if [ ! -d "/sys/fs/cgroup" ]; then
        return 0
    fi

    if [ "$(id -u)" -ne 0 ]; then
        # Non-root: skip cgroup setup
        return 0
    fi

    local cgroup_path="/sys/fs/cgroup/mk_os"

    # Read limits from config or use defaults
    local mem_limit_mb=4096
    local cpu_limit=80

    if [ -f "${CONFIG_FILE}" ]; then
        local mem_from_config
        mem_from_config=$(grep -o '"memory_mb"[[:space:]]*:[[:space:]]*[0-9]*' "${CONFIG_FILE}" 2>/dev/null | grep -o '[0-9]*$' || echo "")
        if [ -n "${mem_from_config}" ]; then
            mem_limit_mb="${mem_from_config}"
        fi
        local cpu_from_config
        cpu_from_config=$(grep -o '"cpu_percent"[[:space:]]*:[[:space:]]*[0-9]*' "${CONFIG_FILE}" 2>/dev/null | grep -o '[0-9]*$' || echo "")
        if [ -n "${cpu_from_config}" ]; then
            cpu_limit="${cpu_from_config}"
        fi
    fi

    if mkdir -p "${cgroup_path}" 2>/dev/null; then
        # Set memory limit
        local mem_bytes=$((mem_limit_mb * 1024 * 1024))
        echo "${mem_bytes}" > "${cgroup_path}/memory.max" 2>/dev/null || true

        # Set CPU limit (cpu_limit% of one core, in microseconds per 100ms period)
        local cpu_quota=$((cpu_limit * 1000))
        echo "${cpu_quota} 100000" > "${cgroup_path}/cpu.max" 2>/dev/null || true

        echo "[MK] Cgroups configured: mem=${mem_limit_mb}MB, cpu=${cpu_limit}%"
    fi
}

# ============================================================
# Source Configuration
# ============================================================
source_config() {
    # Ensure config directory exists
    mkdir -p "${CONFIG_DIR}"

    # Copy default config if none exists
    if [ ! -f "${CONFIG_FILE}" ]; then
        if [ -f "${SCRIPT_DIR}/config/default_config.json" ]; then
            cp "${SCRIPT_DIR}/config/default_config.json" "${CONFIG_FILE}"
            echo "[MK] Default configuration created at ${CONFIG_FILE}"
        fi
    fi

    # Export environment variables MK might need
    if [ -f "${CONFIG_FILE}" ]; then
        # Extract telegram token if present
        local token
        token=$(grep -o '"telegram_token"[[:space:]]*:[[:space:]]*"[^"]*"' "${CONFIG_FILE}" 2>/dev/null | grep -o '"[^"]*"$' | tr -d '"' || echo "")
        if [ -n "${token}" ]; then
            export MK_TELEGRAM_TOKEN="${token}"
        fi
    fi
}

# ============================================================
# Start Background Monitors
# ============================================================
start_monitors() {
    # Start Python device monitor if available
    local monitor_script="${SCRIPT_DIR}/tools/device_monitor.py"
    if [ -f "${monitor_script}" ] && command -v python3 >/dev/null 2>&1; then
        python3 "${monitor_script}" &
        MONITOR_PID=$!
        echo "[MK] Device monitor started (PID: ${MONITOR_PID})"
    fi
}

# ============================================================
# Graceful Shutdown Handler
# ============================================================
cleanup() {
    echo ""
    echo "[MK] Shutdown signal received. Cleaning up..."

    # Stop device monitor
    if [ -n "${MONITOR_PID}" ] && kill -0 "${MONITOR_PID}" 2>/dev/null; then
        kill "${MONITOR_PID}" 2>/dev/null || true
        wait "${MONITOR_PID}" 2>/dev/null || true
        echo "[MK] Device monitor stopped."
    fi

    # Remove PID file
    rm -f "${PID_FILE}"

    echo "[MK] Cleanup complete. Goodbye."
    exit 0
}

# Register signal handlers
trap cleanup SIGINT SIGTERM SIGHUP

# ============================================================
# Main Execution
# ============================================================
main() {
    echo "============================================"
    echo "  MK OS - Enhanced Startup"
    echo "============================================"
    echo ""

    # Step 1: Check resources
    check_resources

    # Step 2: Source configuration
    source_config

    # Step 3: Setup cgroups (if available)
    setup_cgroups

    # Step 4: Start background monitors
    start_monitors

    # Step 5: Check binary exists
    if [ ! -f "${MK_BINARY}" ]; then
        echo "[MK] ERROR: Binary not found at ${MK_BINARY}"
        echo "[MK] Run 'make' in the MK_OS directory first."
        exit 1
    fi

    # Step 6: Record PID
    echo "$$" > "${PID_FILE}"

    # Step 7: Launch MK binary
    echo ""
    echo "[MK] Launching MK OS..."
    echo ""

    cd "${SCRIPT_DIR}"
    exec "${MK_BINARY}"
}

main "$@"
