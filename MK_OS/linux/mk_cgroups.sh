#!/bin/bash
# ============================================================
# MK OS - cgroups v2 Resource Limiter
# Sets up cgroups v2 resource limits for the MK OS process.
# Configurable via environment variables.
# ============================================================

set -euo pipefail

# Configuration (override via environment variables)
MK_MEM_LIMIT="${MK_MEM_LIMIT:-4G}"
MK_CPU_PERCENT="${MK_CPU_PERCENT:-80}"
MK_CGROUP_NAME="${MK_CGROUP_NAME:-mk_os.slice}"
CGROUP_BASE="/sys/fs/cgroup"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info() {
    echo -e "${GREEN}[MK-CGROUPS]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[MK-CGROUPS]${NC} WARNING: $1"
}

log_error() {
    echo -e "${RED}[MK-CGROUPS]${NC} ERROR: $1" >&2
}

# Check if cgroups v2 is available
check_cgroups_v2() {
    if [ ! -f "${CGROUP_BASE}/cgroup.controllers" ]; then
        log_error "cgroups v2 not available at ${CGROUP_BASE}"
        log_error "Ensure your kernel supports cgroups v2 (unified hierarchy)"
        return 1
    fi
    log_info "cgroups v2 detected"
    return 0
}

# Convert human-readable memory to bytes
parse_memory_limit() {
    local input="$1"
    local value
    local unit

    value=$(echo "$input" | sed 's/[^0-9.]//g')
    unit=$(echo "$input" | sed 's/[0-9.]//g' | tr '[:lower:]' '[:upper:]')

    case "$unit" in
        K|KB) echo $(( ${value%.*} * 1024 )) ;;
        M|MB) echo $(( ${value%.*} * 1024 * 1024 )) ;;
        G|GB) echo $(( ${value%.*} * 1024 * 1024 * 1024 )) ;;
        T|TB) echo $(( ${value%.*} * 1024 * 1024 * 1024 * 1024 )) ;;
        *)    echo "$value" ;;
    esac
}

# Create the MK OS cgroup slice
create_cgroup() {
    local cgroup_path="${CGROUP_BASE}/${MK_CGROUP_NAME}"

    if [ -d "$cgroup_path" ]; then
        log_info "cgroup ${MK_CGROUP_NAME} already exists"
        return 0
    fi

    if ! mkdir -p "$cgroup_path" 2>/dev/null; then
        log_error "Failed to create cgroup at ${cgroup_path}"
        log_error "Are you running as root?"
        return 1
    fi

    log_info "Created cgroup: ${MK_CGROUP_NAME}"
    return 0
}

# Set memory limit
set_memory_limit() {
    local cgroup_path="${CGROUP_BASE}/${MK_CGROUP_NAME}"
    local mem_bytes

    mem_bytes=$(parse_memory_limit "$MK_MEM_LIMIT")

    if [ -f "${cgroup_path}/memory.max" ]; then
        echo "$mem_bytes" > "${cgroup_path}/memory.max"
        log_info "Memory limit set to ${MK_MEM_LIMIT} (${mem_bytes} bytes)"
    else
        log_warn "memory.max not available in cgroup"
    fi

    # Set high watermark at 90% of max for early warning
    local high_bytes=$(( mem_bytes * 90 / 100 ))
    if [ -f "${cgroup_path}/memory.high" ]; then
        echo "$high_bytes" > "${cgroup_path}/memory.high"
        log_info "Memory high watermark set to 90% of max"
    fi
}

# Set CPU limit
set_cpu_limit() {
    local cgroup_path="${CGROUP_BASE}/${MK_CGROUP_NAME}"

    # cpu.max format: "quota period" (in microseconds)
    # 100000 = 100ms period (standard)
    local period=100000
    local quota=$(( period * MK_CPU_PERCENT / 100 ))

    if [ -f "${cgroup_path}/cpu.max" ]; then
        echo "${quota} ${period}" > "${cgroup_path}/cpu.max"
        log_info "CPU limit set to ${MK_CPU_PERCENT}% (${quota}/${period} us)"
    else
        log_warn "cpu.max not available in cgroup"
    fi
}

# Add a PID to the cgroup
add_pid() {
    local pid="$1"
    local cgroup_path="${CGROUP_BASE}/${MK_CGROUP_NAME}"

    if [ ! -f "${cgroup_path}/cgroup.procs" ]; then
        log_error "Cannot add PID - cgroup.procs not found"
        return 1
    fi

    echo "$pid" > "${cgroup_path}/cgroup.procs"
    log_info "Added PID ${pid} to ${MK_CGROUP_NAME}"
}

# Show current cgroup status
show_status() {
    local cgroup_path="${CGROUP_BASE}/${MK_CGROUP_NAME}"

    if [ ! -d "$cgroup_path" ]; then
        log_warn "cgroup ${MK_CGROUP_NAME} does not exist"
        return 1
    fi

    echo ""
    log_info "=== MK OS cgroup Status ==="
    echo ""

    if [ -f "${cgroup_path}/memory.max" ]; then
        echo "  Memory limit:   $(cat "${cgroup_path}/memory.max")"
    fi
    if [ -f "${cgroup_path}/memory.current" ]; then
        echo "  Memory current: $(cat "${cgroup_path}/memory.current")"
    fi
    if [ -f "${cgroup_path}/cpu.max" ]; then
        echo "  CPU max:        $(cat "${cgroup_path}/cpu.max")"
    fi
    if [ -f "${cgroup_path}/cgroup.procs" ]; then
        local pids
        pids=$(wc -l < "${cgroup_path}/cgroup.procs")
        echo "  Active PIDs:    ${pids}"
    fi
    echo ""
}

# Main setup routine
setup() {
    log_info "Setting up cgroups for MK OS..."
    log_info "Memory limit: ${MK_MEM_LIMIT}"
    log_info "CPU limit: ${MK_CPU_PERCENT}%"
    echo ""

    check_cgroups_v2 || exit 1
    create_cgroup || exit 1
    set_memory_limit
    set_cpu_limit

    echo ""
    log_info "cgroup setup complete!"
    show_status
}

# Parse command line arguments
case "${1:-setup}" in
    setup)
        setup
        ;;
    status)
        show_status
        ;;
    add-pid)
        if [ -z "${2:-}" ]; then
            log_error "Usage: $0 add-pid <PID>"
            exit 1
        fi
        add_pid "$2"
        ;;
    *)
        echo "Usage: $0 {setup|status|add-pid <PID>}"
        echo ""
        echo "Environment variables:"
        echo "  MK_MEM_LIMIT    - Memory limit (default: 4G)"
        echo "  MK_CPU_PERCENT  - CPU percentage limit (default: 80)"
        echo "  MK_CGROUP_NAME  - cgroup slice name (default: mk_os.slice)"
        exit 1
        ;;
esac
