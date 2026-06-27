#!/bin/sh
# =============================================================================
# MK OS - VM Init Script
# =============================================================================
# This script runs inside the Alpine Linux VM on boot.
# It sets up the environment and launches the MK OS binary.
# Called as part of the boot sequence or directly for debugging.
# =============================================================================

LOG_FILE="/var/log/mk_boot.log"
MK_DIR="/opt/mk_os"
MK_BINARY="${MK_DIR}/mk_os"
MK_PID_FILE="/var/run/mk_os.pid"
NETWORK_SCRIPT="${MK_DIR}/network_setup.sh"

# --- Logging ---
log() {
    local msg="[MK INIT] $(date '+%Y-%m-%d %H:%M:%S') $*"
    echo "$msg"
    echo "$msg" >> "${LOG_FILE}"
}

# --- Mount required filesystems ---
mount_filesystems() {
    log "Mounting required filesystems..."

    if ! mountpoint -q /proc 2>/dev/null; then
        mount -t proc proc /proc
        log "  /proc mounted"
    fi

    if ! mountpoint -q /sys 2>/dev/null; then
        mount -t sysfs sysfs /sys
        log "  /sys mounted"
    fi

    if ! mountpoint -q /dev 2>/dev/null; then
        mount -t devtmpfs devtmpfs /dev 2>/dev/null || \
        mount -t tmpfs tmpfs /dev
        log "  /dev mounted"
    fi

    # Mount devpts for terminal support
    mkdir -p /dev/pts
    if ! mountpoint -q /dev/pts 2>/dev/null; then
        mount -t devpts devpts /dev/pts 2>/dev/null || true
    fi

    # Mount tmpfs on /tmp and /run
    mount -t tmpfs tmpfs /tmp 2>/dev/null || true
    mount -t tmpfs tmpfs /run 2>/dev/null || true

    log "Filesystems mounted successfully."
}

# --- Set up networking ---
setup_network() {
    log "Setting up networking..."

    if [ -x "${NETWORK_SCRIPT}" ]; then
        "${NETWORK_SCRIPT}" >> "${LOG_FILE}" 2>&1
    else
        # Fallback: basic DHCP setup
        log "  Network script not found, using fallback..."
        ip link set lo up
        ip link set eth0 up 2>/dev/null || true
        udhcpc -i eth0 -q -n 2>/dev/null || \
            dhclient eth0 2>/dev/null || \
            log "  WARNING: DHCP failed. Network may not be available."
    fi

    log "Network setup complete."
}

# --- Set hostname ---
set_hostname() {
    log "Setting hostname to mk-os..."
    hostname mk-os
    echo "mk-os" > /etc/hostname
}

# --- Create required directories ---
create_directories() {
    log "Creating MK OS directories..."
    mkdir -p "${MK_DIR}"
    mkdir -p "${MK_DIR}/knowledge"
    mkdir -p /var/log
    mkdir -p /var/run
    log "Directories ready."
}

# --- Signal handling ---
shutdown_handler() {
    log "Received shutdown signal. Stopping MK OS..."
    if [ -f "${MK_PID_FILE}" ]; then
        MK_PID=$(cat "${MK_PID_FILE}")
        if kill -0 "${MK_PID}" 2>/dev/null; then
            kill -TERM "${MK_PID}"
            # Wait up to 10 seconds for graceful shutdown
            WAIT=0
            while kill -0 "${MK_PID}" 2>/dev/null && [ ${WAIT} -lt 10 ]; do
                sleep 1
                WAIT=$((WAIT + 1))
            done
            if kill -0 "${MK_PID}" 2>/dev/null; then
                log "Force killing MK OS..."
                kill -9 "${MK_PID}"
            fi
        fi
        rm -f "${MK_PID_FILE}"
    fi
    log "MK OS stopped. Shutting down."
    exit 0
}

trap shutdown_handler TERM INT HUP

# --- Start MK OS with auto-restart ---
start_mk_os() {
    log "Starting MK OS binary..."

    if [ ! -x "${MK_BINARY}" ]; then
        log "ERROR: MK OS binary not found or not executable at ${MK_BINARY}"
        log "Attempting to fix permissions..."
        chmod +x "${MK_BINARY}" 2>/dev/null || true
        if [ ! -x "${MK_BINARY}" ]; then
            log "FATAL: Cannot execute MK OS binary. Dropping to shell."
            exec /bin/sh
        fi
    fi

    # Main loop: restart MK OS if it crashes
    while true; do
        log "Launching MK OS (PID will be recorded)..."

        cd "${MK_DIR}"
        "${MK_BINARY}" >> /var/log/mk_os.log 2>&1 &
        MK_PID=$!
        echo "${MK_PID}" > "${MK_PID_FILE}"
        log "MK OS started with PID ${MK_PID}"

        # Wait for the process to exit
        wait "${MK_PID}"
        EXIT_CODE=$?

        log "MK OS exited with code ${EXIT_CODE}"

        if [ ${EXIT_CODE} -eq 0 ]; then
            log "MK OS exited cleanly. Not restarting."
            break
        fi

        log "MK OS crashed! Restarting in 3 seconds..."
        rm -f "${MK_PID_FILE}"
        sleep 3
    done
}

# =============================================================================
# MAIN EXECUTION
# =============================================================================

log "=========================================="
log "  MK OS Boot Sequence Starting"
log "  Date: $(date)"
log "  Kernel: $(uname -r)"
log "  Arch: $(uname -m)"
log "=========================================="

# Step 1: Mount filesystems
mount_filesystems

# Step 2: Set hostname
set_hostname

# Step 3: Create directories
create_directories

# Step 4: Set up networking
setup_network

# Step 5: Start MK OS (with crash recovery)
start_mk_os

log "MK OS init complete. System idle."
