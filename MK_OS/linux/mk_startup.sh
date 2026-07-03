#!/bin/bash
# ============================================================
# MK OS - Safe Startup Script
# Checks hardware resources before starting MK daemon.
# Verifies: RAM > 512MB, disk > 1GB free, sets up cgroups.
# ============================================================

set -euo pipefail

MK_HOME="${MK_HOME:-/opt/mk_os}"
MK_BIN="${MK_HOME}/mk_os"
MK_CONFIG="${MK_HOME}/mk_os.env"
MK_LOG="/var/log/mk_os.log"
MIN_RAM_MB=512
MIN_DISK_MB=1024

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() { echo -e "${GREEN}[MK-BOOT]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[MK-BOOT]${NC} $1"; }
log_error() { echo -e "${RED}[MK-BOOT]${NC} $1" >&2; }

# Check available RAM
check_ram() {
    local available_kb=0
    if [ -f /proc/meminfo ]; then
        available_kb=$(grep '^MemAvailable:' /proc/meminfo | awk '{print $2}')
    fi
    local available_mb=$(( available_kb / 1024 ))

    if [ "$available_mb" -lt "$MIN_RAM_MB" ]; then
        log_error "Insufficient RAM: ${available_mb}MB available, need ${MIN_RAM_MB}MB"
        return 1
    fi
    log_info "RAM check passed: ${available_mb}MB available"
    return 0
}

# Check free disk space
check_disk() {
    local free_kb
    free_kb=$(df -k "$MK_HOME" 2>/dev/null | tail -1 | awk '{print $4}')
    local free_mb=$(( free_kb / 1024 ))

    if [ "$free_mb" -lt "$MIN_DISK_MB" ]; then
        log_error "Insufficient disk: ${free_mb}MB free, need ${MIN_DISK_MB}MB"
        return 1
    fi
    log_info "Disk check passed: ${free_mb}MB free"
    return 0
}

# Source configuration
load_config() {
    if [ -f "$MK_CONFIG" ]; then
        # shellcheck source=/dev/null
        source "$MK_CONFIG"
        log_info "Configuration loaded from ${MK_CONFIG}"
    else
        log_warn "No config file found at ${MK_CONFIG}, using defaults"
    fi
}

# Setup cgroups
setup_cgroups() {
    local cgroups_script="${MK_HOME}/linux/mk_cgroups.sh"
    if [ -x "$cgroups_script" ]; then
        log_info "Setting up cgroups..."
        "$cgroups_script" setup || log_warn "cgroups setup failed (non-fatal)"
    else
        log_warn "cgroups script not found or not executable"
    fi
}

# Start the MK daemon
start_daemon() {
    if [ ! -x "$MK_BIN" ]; then
        log_error "MK binary not found or not executable: ${MK_BIN}"
        return 1
    fi

    log_info "Starting MK OS daemon..."
    cd "$MK_HOME"

    if [ "${MK_DAEMON:-false}" = "true" ]; then
        nohup "$MK_BIN" >> "$MK_LOG" 2>&1 &
        local pid=$!
        echo "$pid" > "${MK_HOME}/mk_os.pid"
        log_info "MK daemon started (PID: ${pid})"
    else
        exec "$MK_BIN"
    fi
}

# Main startup sequence
main() {
    echo ""
    log_info "=== MK OS Safe Startup ==="
    echo ""

    # Run pre-flight checks
    check_ram || exit 1
    check_disk || exit 1

    # Load configuration
    load_config

    # Setup resource limits
    setup_cgroups

    # Start the daemon
    start_daemon
}

main "$@"
